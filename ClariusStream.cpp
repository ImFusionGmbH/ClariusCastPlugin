#include "ClariusStream.h"

#include "ClariusApi.h"

#include <ImFusion/Base/IMUPoseIntegration.h>
#include <ImFusion/Base/ImageProcessing.h>
#include <ImFusion/Base/TypedImage.h>
#include <ImFusion/Base/Utils/DataLogger.h>
#include <ImFusion/Core/Log.h>
#include <ImFusion/GL/SharedImage.h>
#include <ImFusion/Stream/ImageStreamData.h>
#include <ImFusion/US/FrameGeometryConvex.h>
#include <ImFusion/US/FrameGeometryLinear.h>
#include <ImFusion/US/FrameGeometryMetadata.h>
#include <ImFusion/US/GeometryDetection.h>
#include <ImFusion/US/UltrasoundMetadata.h>

#include <boost/lockfree/queue.hpp>

#include <future>

#undef IMFUSION_LOG_DEFAULT_CATEGORY
#define IMFUSION_LOG_DEFAULT_CATEGORY "ClariusStream"


namespace ImFusion
{
	namespace
	{
		unsigned int hash(const unsigned char* data, int size)
		{
			int h = 0;
			for (int i = 0; i < size; i++)
				h = h * 33 + ((data[i] == 255) * i) % 701;
			return h;
		}
	}

	ClariusStream* ClariusStream::m_singletonStreamInstance = nullptr;

	struct ClariusStream::Impl
	{
		std::future<void> processingThread;           ///< Future wrapping the data processing thread.
		std::condition_variable conditionVariable;    ///< Condition variable for notification of the processing thread
		std::atomic<bool> stopExecution = {false};    ///< Flag whether to stop the execution of the processing thread.
		std::mutex processingThreadMutex;             ///< Mutex protecting access to conditionVariable
		boost::lockfree::queue<ImageStreamData*, boost::lockfree::capacity<50>> scanDataBuffer;    ///< Thread-safe queue for scan data messages
	};

	ClariusStream::ClariusStream(const std::string& name, bool useCastApi)
		: ImageStream(name)
		, m_pimpl(new Impl())
	{
		setModality(Data::ULTRASOUND);

		m_api = ClariusCastApi::get();

		IMFUSION_ASSERT(m_api);

		m_api->imageCallback =
			[this](std::unique_ptr<TypedImage<unsigned char>>&& img, unsigned long long timestamp, std::unique_ptr<IMURawMetadata>&& imu) {
				auto mask = TypedImage<unsigned char>::create(vec3i(img->width(), img->height(), 1), 1);
				mask->setSpacing(img->spacing(), true);
				for (int i = 0; i < img->width() * img->height(); i++)
					mask->pointer()[i] = img->pointer()[i * 4 + 3];

				unsigned int maskHash = hash(mask->pointer(), img->width() * img->height());

				if (m_previousWidth != img->width() || m_previousHeight != img->height() || maskHash != m_previousMaskHash)
					m_geometry.reset();

				m_previousWidth = img->width();
				m_previousHeight = img->height();
				m_previousMaskHash = maskHash;

				if (m_geometry == nullptr)
				{
					// make sure we don't run it on every frame if it fails
					if (m_lastGeometryDetectionHash != maskHash)
					{
						US::GeometryDetection det;
						m_geometry = det.compute(mask.get());
						m_lastGeometryDetectionHash = maskHash;
					}
				}

				if (!m_isRunning)
					return;

				const std::string probeID = "Clarius";

				std::shared_ptr<SharedImage> si = std::make_shared<SharedImage>(std::move(img));
				auto* isd = new ImageStreamData(this,
												si);    // we're transferring ownership here - make sure to delete the image afterward!
				isd->setTimestampArrival(std::chrono::system_clock::now());
				isd->setTimestampDevice(static_cast<uint64_t>(timestamp / 1e6));    // ns to ms

				auto metaUS = std::make_unique<US::UltrasoundMetadata>();
				metaUS->m_device = probeID;
				metaUS->m_probe = probeID;
				metaUS->m_endDepth = si->mem()->extent().y();
				metaUS->m_focalDepth = metaUS->m_endDepth / 2;
				metaUS->m_scanConverted = true;
				isd->components().add(std::move(metaUS));

				if (m_geometry)
				{
					auto metaGeom = std::make_unique<US::FrameGeometryMetadata>();
					metaGeom->setFrameGeometry(m_geometry->clone());
					isd->components().add(std::move(metaGeom));
				}

				if (imu)
					isd->components().add(std::move(imu));

				if (m_pimpl->scanDataBuffer.push(isd))
					m_pimpl->conditionVariable.notify_one();    // wake up processing thread
				else
				{
					LOG_WARN("ClariusStream::onImageArrived: buffer full! Clearing buffer...");
					clearBuffer();
				}
			};

		m_api->measuresCallback = [this](double depth, double width) { m_measuredDepth = depth; };

		m_api->buttonCallback = [this](int button, int clicks) { buttonPressed.emitSignal(button); };

		m_api->freezeCallback = [this](bool frozen) {
			if (frozen)
				pause();
			else
				resume();
		};

		// Launch the processing thread
		m_pimpl->processingThread = std::async(std::launch::async, [this]() {
			try
			{
				std::unique_lock<std::mutex> lock(m_pimpl->processingThreadMutex);

				while (!m_pimpl->stopExecution)
				{
					while (!m_pimpl->scanDataBuffer.empty())
					{
						ImageStreamData* isd;
						if (!m_pimpl->scanDataBuffer.pop(isd))    // it's possible that while waiting for the lock, the queue has been cleared
							break;

						if (p_convertToGray)
						{
							auto imgs = isd->images2();
							auto newmem = ImageProcessing::createGrayscale(*imgs[0]->mem(), 3);
							isd->setImages({std::make_shared<SharedImage>(std::move(newmem))});
						}

						signalNewData.emitSignal(*isd);
						delete isd;
					}

					if (!m_pimpl->stopExecution)    // go hibernate
					{
						m_pimpl->conditionVariable.wait(lock);
					}
				}
			}
			catch (std::exception& e)
			{
				LOG_ERROR("An unexpected exception occurred while running the background thread. " << e.what());
			}
		});
	}


	ClariusStream::~ClariusStream()
	{
		close();
		try
		{
			m_api->destroy();
		}
		catch (...)
		{
		}

		// Let the background thread gracefully quit
		{
			std::unique_lock<std::mutex> lock(m_pimpl->processingThreadMutex);
			m_pimpl->stopExecution = true;
		}
		while (m_pimpl->processingThread.wait_for(std::chrono::milliseconds(100)) != std::future_status::ready)
			m_pimpl->conditionVariable.notify_one();

		clearBuffer();
	}


	bool ClariusStream::canInstantiate() { return m_singletonStreamInstance == nullptr; }


	bool ClariusStream::openImpl()
	{
		m_singletonStreamInstance = this;

		try
		{
			if (!m_api->init())
			{
				LOG_ERROR("Could not initialize Clarius listener");
				return false;
			}

			if (!m_api->connect(p_serverAddress.value().c_str(), p_serverPort.value()))
			{
				LOG_ERROR("Could not connect to Clarius device");
				return false;
			}

			m_isInitialized = true;
			LOG_INFO("Clarius connection established to " << p_serverAddress.value() << ", awaiting incoming UDP data");
		}
		catch (...)
		{
			LOG_ERROR("Could not connect to Clarius device, unknown exception");
			return false;
		}
		return true;
	}


	bool ClariusStream::closeImpl()
	{
		m_api->disconnect();

		m_isInitialized = false;
		m_isRunning = false;
		return true;
	}


	bool ClariusStream::startImpl()
	{
		if (!m_isInitialized)
		{
			LOG_ERROR("Clarius connection is not initialized yet.");
			return false;
		}

		if (m_isRunning)
			return true;

		LOG_INFO("Clarius US image stream started");
		m_isRunning = true;

		return m_api->start();
	}


	bool ClariusStream::stopImpl()
	{
		if (!m_isInitialized)
		{
			LOG_ERROR("Clarius connection is not initialized yet.");
			return false;
		}

		m_isRunning = false;
		return true;
	}

	void ClariusStream::onImageArrived(std::unique_ptr<MemImage> mem, unsigned long long imgTm, std::unique_ptr<IMURawMetadata> imuMetadata) {}


	std::string ClariusStream::uuid()
	{
		std::stringstream ss;
		ss << this;
		return ss.str();
	}

	void ClariusStream::configure(const Properties* p)
	{
		std::unique_lock<std::mutex> lock(m_pimpl->processingThreadMutex);
		ImageStream::configure(p);
	}

	void ClariusStream::clearBuffer()
	{
		ImageStreamData* tmp;
		while (m_pimpl->scanDataBuffer.pop(tmp))
			delete tmp;
	}
}
