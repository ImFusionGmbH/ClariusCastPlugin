#include "ClariusController.h"

#include "ClariusStream.h"

#include <ImFusion/Core/Log.h>
#include <ImFusion/GL/GlSliceView.h>
#include <ImFusion/GL/GlImage.h>
#include <ImFusion/GL/SharedImageSet.h>
#include <ImFusion/GUI/DisplayWidgetMulti.h>
#include <ImFusion/GUI/ImageView2D.h>
#include <ImFusion/GUI/PropertiesWidget.h>

#include <QPushButton>
#include <QLabel>

namespace ImFusion
{
	ClariusController::ClariusController(ClariusStreamIoAlgorithm* alg)
		: StreamControllerBase(alg, false)
	{
		m_propertiesWidget = new PropertiesWidget(nullptr, nullptr, this);
		m_layout = new QVBoxLayout();
		setLayout(m_layout);
		m_layout->addWidget(m_propertiesWidget);
		connect(m_propertiesWidget, SIGNAL(parameterChanged(std::string)), this, SLOT(onParameterChanged()));

		auto* hor = new QHBoxLayout();
		auto* horWidget = new QWidget(this);
		horWidget->setLayout(hor);
		m_startStopButton = new QPushButton("Start");
		hor->addWidget(m_startStopButton);

		m_fpsLabel = new QLabel("");
		hor->addWidget(m_fpsLabel);

		m_propertiesWidget->setSplitCamelCase(true);
		m_layout->addWidget(horWidget);
		m_layout->addWidget(m_propertiesWidget);
	}


	ClariusController::~ClariusController() = default;

	void ClariusController::init()
	{
		StreamControllerBase::init();
		m_clariusStream = dynamic_cast<ClariusStream*>(m_stream);
		IMFUSION_ASSERT(m_clariusStream);

		m_fps.connect(m_clariusStream);
		//m_clariusStream->signalNewData.connect(this, [this](const StreamData& sd) { QMetaObject::invokeMethod(this, "controllerUpdate", Qt::QueuedConnection); });
		m_clariusStream->signalStateChanged.connect(this, [this](Stream::StateChange _) { QMetaObject::invokeMethod(this, "controllerUpdate", Qt::QueuedConnection); });

		connect(m_startStopButton, SIGNAL(clicked()), this, SLOT(onStartStop()));
		m_startStopButton->setCheckable(true);

		m_fps.setNumberOfFrames(30);    // It takes longer to update frame rate but the value is more stable

		connect(m_propertiesWidget, SIGNAL(parameterChanged(std::string)), this, SLOT(updateFlip()));

		// ensure that we have a 2D view (not necessarily the case because of DataGroup)
		m_disp->view2D()->setVisible(true);
		controllerUpdate();

		connect(&m_timer, &QTimer::timeout, this, &ClariusController::onUpdateStatus);
		m_timer.start(500);
	}

	void ClariusController::controllerUpdate()
	{
		std::vector<Stream::State> runningStates = { Stream::State::Running, Stream::State::Starting, Stream::State::Pausing, Stream::State::Paused, Stream::State::Resuming };
		if (std::find(runningStates.begin(), runningStates.end(), m_clariusStream->currentState()) != runningStates.end())
		{
			m_startStopButton->setText("Stop");
			m_startStopButton->blockSignals(true);
			m_startStopButton->setChecked(true);
			m_startStopButton->blockSignals(false);
		}
		else
		{
			m_startStopButton->setText("Start");
			m_startStopButton->blockSignals(true);
			m_startStopButton->setChecked(false);
			m_startStopButton->blockSignals(false);
		}
	}


	void ClariusController::onStartStop()
	{
		bool running = m_startStopButton->text() == "Stop";

		if (running)
		{
			m_clariusStream->stop();
		}
		else
		{
			// try to connect if this hasn't happened yet
			if (!m_clariusStream->isInitialized())
			{
				if (!m_clariusStream->open())
					return;
			}
			m_clariusStream->start();
		}
		if (running != m_clariusStream->isRunning())
			controllerUpdate();
	}

	void ClariusController::updateFlip()
	{
		if (!m_disp->view2D() || !m_disp->view2D()->view())
			return;

		if (m_clariusStream->p_flipView)
		{
			mat4 A = mat4::Identity();
			A.block<3, 3>(0, 0) = mat3(Eigen::AngleAxisd(M_PI, vec3::UnitY()));
			m_disp->view2D()->view()->setMatrix(A);
		}
		else
		{
			m_disp->view2D()->view()->setMatrix(mat4::Identity());
		}
	}

	void ClariusController::onUpdateStatus()
	{
		double fps = m_fps.fps();
		// Since we are in the main thread, query the 2D image texture, same as for adjusting resolution
		int width = 0, height = 0;
		if (auto is = m_disp->view2D()->view()->slice()->imageData())
		{
			if (const GlImage* img = is->gl())
			{
				width = img->width();
				height = img->height();
			}
		}
		if ((fps > 1) && (width > 0) && (height > 0))
			m_fpsLabel->setText(QString("Resolution %1 x %2 px, FPS %3").arg(width).arg(height).arg(fps));
		else
			m_fpsLabel->clear();
	}
}
