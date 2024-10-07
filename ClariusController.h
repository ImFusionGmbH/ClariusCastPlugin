/* Copyright (c) 2012-2019 ImFusion GmbH, Munich, Germany. All rights reserved. */
#pragma once

#include "ClariusStream.h"
#include "ClariusStreamIoAlgorithm.h"

#include <ImFusion/Stream/StreamControllerBase.h>
#include <ImFusion/Stream/StreamFps.h>

#include <QtCore/QTimer>
#include <QtWidgets/QWidget>

class QPushButton;
class QLabel;

namespace ImFusion
{
	class ClariusStream;
	class StreamData;
	class InteractiveObject;

	/** \brief	Stream controller for handling a Clarius system
	 *	\author	Oliver Zettinig
	 */
	class ClariusController : public StreamControllerBase
	{
		Q_OBJECT
	public:
		explicit ClariusController(ClariusStreamIoAlgorithm* alg);

		/// Destructor
		~ClariusController();

		/// Initializes the widget
		void init() override;

	private slots:
		/// Updates UI
		void controllerUpdate();

		/// Start or stop the US stream
		void onStartStop();

		/// Switch between horizontal flips of the view
		void updateFlip();

		void onUpdateStatus();

	private:
		ClariusStream* m_clariusStream = nullptr;    ///< Stream instance which communicates directly to the Clarius API
		StreamFps m_fps;                             ///< Frames per second counter
		QTimer m_timer;                              ///< Timer for status update

		QPushButton* m_startStopButton;
		QLabel* m_fpsLabel;
	};
}
