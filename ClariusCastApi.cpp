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
#include <ImFusion/US/UltrasoundGeometryDetection.h>
#include <ImFusion/US/UltrasoundMetadata.h>

#include <cast/cast.h>

#include <QDir>

namespace ImFusion
{
	ClariusCastApi* ClariusCastApi::m_singletonCastApiInstance = nullptr;

	ClariusCastApi* ClariusCastApi::get()
	{
		if (m_singletonCastApiInstance == nullptr)
			m_singletonCastApiInstance = new ClariusCastApi();
		return m_singletonCastApiInstance;
	}

	ClariusCastApi::ClariusCastApi() { IMFUSION_ASSERT(m_singletonCastApiInstance == nullptr); }

	bool ClariusCastApi::init()
	{
		auto tmpPath = QDir::tempPath().toStdString();

		int res = cusCastInit(
			0,
			nullptr,
			tmpPath.c_str(),
			// New processed image function callback
			[](const void* newImage, const CusProcessedImageInfo* nfo, int npos, const CusPosInfo* pos) {
				try
				{
					if (!nfo)
						return;
					const int channels = nfo->bitsPerPixel / 8;
					ImageDescriptor desc(PixelType::UByte, vec3i(nfo->width, nfo->height, 1), channels);
					auto img = TypedImage<unsigned char>::create(desc);
					memcpy(img->data(), newImage, sizeof(unsigned char) * nfo->width * nfo->height * channels);
					img->setSpacing(nfo->micronsPerPixel * 1.e-3, nfo->micronsPerPixel * 1.e-3, 1., true);

					std::unique_ptr<IMURawMetadata> imuMetadata;
					if (npos > 0 && pos)
					{
						imuMetadata = std::make_unique<IMURawMetadata>();
						imuMetadata->m_samples.resize(npos);
						for (int i = 0; i < npos; i++)
						{
							imuMetadata->m_samples[i].gyro = vec3(pos[i].gx, pos[i].gy, pos[i].gz);
							imuMetadata->m_samples[i].linAcc = vec3(pos[i].ax, pos[i].ay, pos[i].az);
							imuMetadata->m_samples[i].mag = vec3(pos[i].mx, pos[i].my, pos[i].mz);
							imuMetadata->m_samples[i].timestamp = static_cast<unsigned long long>(pos[i].tm);
						}
					}

					if (m_singletonCastApiInstance)
						m_singletonCastApiInstance->imageCallback(
							std::move(img), static_cast<unsigned long long>(nfo->tm), std::move(imuMetadata));
				}
				catch (...)
				{
					LOG_ERROR("Undefined error in new image function callback.");
				}
			},
			// New raw image function callback
				[](const void* /*newImage*/, const CusRawImageInfo* nfo, int /*npos*/, const CusPosInfo* /*pos*/) {
				double measuredDepth = std::floor((nfo->samples * nfo->axialSize) * 1e-3);
				if (m_singletonCastApiInstance)
					m_singletonCastApiInstance->measuresCallback(measuredDepth, nfo->lines * nfo->lateralSize * 1e-3);
			},
				// New spectral image function callback
				[](const void* /*newImage*/, const CusSpectralImageInfo* /*nfo*/) {},
				// New IMU data callback
				[](const CusPosInfo* pos) {},
				// Freeze / unfreeze function callback
				[](int val) {
				if (m_singletonCastApiInstance)
					m_singletonCastApiInstance->freezeCallback(val == 1);    // 1 = frozen, 0 = imaging
			},
				// Button function callback
				[](CusButton btn, int clicks) {
				if (m_singletonCastApiInstance)
					m_singletonCastApiInstance->buttonCallback(static_cast<int>(btn), clicks);
			},
				// Progress function callback
				[](int /*progress*/) {},
				// Error function callback
				[](const char* err) { LOG_ERROR("Clarius Cast reported error: " << err); },
				// Return function callback - nullptr will block
				// nullptr,
				640,
				480);

		if (res < 0)
			return false;

		char firmwareVersion[32];
		res = cusCastFwVersion(CusPlatform::HD3, firmwareVersion, 32);
		LOG_INFO("Retrieved firmware version: " << firmwareVersion << "(" << res << ")");
		return true;
	}

	bool ClariusCastApi::connect(const char* ipAddress, unsigned int port)
	{
		return cusCastConnect(ipAddress, port, "research", [](int port, int swRevMatch)
			{
				if (swRevMatch == CUS_SUCCESS)
					LOG_INFO("Clarius Cast version matches. Suite was build with " << CLARIUS_CAST_VERSION);
				else
					LOG_ERROR("Clarius Cast version mismatch, Suite was build with " << CLARIUS_CAST_VERSION);
			}) >= 0;
	}

	void ClariusCastApi::disconnect()
	{
		cusCastDisconnect([](int retCode) {});
	}

	void ClariusCastApi::destroy()
	{
		cusCastDestroy();
	}

	bool ClariusCastApi::setGain(double gain) { return cusCastUserFunction(CusUserFunction::SetGain, gain / 100.0 - 0.5, nullptr) >= 0; }

	bool ClariusCastApi::setDepth(double depth) { return cusCastUserFunction(CusUserFunction::SetDepth, depth / 10.0, nullptr) >= 0; }

	bool ClariusCastApi::setResolution(vec2i resolution) { return cusCastSetOutputSize(resolution[0], resolution[1]) >= 0; }

	bool ClariusCastApi::loadCertificate(const std::string& path) { return true; }
}