/* Copyright (c) 2012-2019 ImFusion GmbH, Munich, Germany. All rights reserved. */
#pragma once

#include <ImFusion/Core/Parameter.h>
#include <ImFusion/Stream/ImageStream.h>

#include <memory>

namespace ImFusion
{
	class MemImage;

	class IMURawMetadata;
	class ClariusApi;

	namespace US
	{
		class FrameGeometry;
	}

	/**	\brief	Stream class for the Clarius ultrasound system
	 *	\author	Oliver Zettinig
	 */
	class ClariusStream : public ImageStream
	{
	public:
		/// Currently, only one instance is allowed
		static bool canInstantiate();

		/// Constructor
		explicit ClariusStream(const std::string& name = "Clarius Stream", bool useCastApi = true);

		/// Destructor
		~ClariusStream() override;

		/// \name Stream Interface Methods
		//\{

		bool isRunning() const override { return m_isRunning; }

		bool isInitialized() const { return m_isInitialized; }

		bool topDown() const override { return true; }

		std::string uuid() override;

		bool supportsPausing() const override { return true; }

		bool pauseImpl() override { m_isRunning = false; return true; }

		bool resumeImpl() override { m_isRunning = true; return true; }

		///\}

		void configure(const Properties* p) override;

		Parameter<std::string> p_serverAddress = { "serverAddress", "", *this };    ///< Host name for listener connection
		Parameter<unsigned int> p_serverPort = { "serverPort", 35583, *this };      ///< Port for listener connection
		Parameter<bool> p_convertToGray = { "convertToGray", false, *this };        ///< If set to true, result images will be converted to greyscale
		Parameter<bool> p_flipView = { "flipView", false, *this };                  ///< If set to true the controller will flip the view

		Signal<int> buttonPressed;

		static ClariusStream* m_singletonStreamInstance;    ///< This is to prevent multiple instances
		/// Process image callback 
		void onImageArrived(std::unique_ptr<MemImage> mem, unsigned long long imgTm, std::unique_ptr<IMURawMetadata> imuMetadata);
	protected:
		/// Creates the USEngine
		bool openImpl() override;

		/// Stops and teardowns the USEngine
		bool closeImpl() override;

		/// Start streaming US images
		bool startImpl() override;

		/// Stop streaming
		bool stopImpl() override;

		std::optional<WorkContinuation> doWork() override { return std::nullopt; }

	private:
		void clearBuffer();
		ClariusApi* m_api;

		struct Impl;
		std::unique_ptr<Impl> m_pimpl;

		// Streaming members
		bool m_isInitialized = false;    ///< True if connection established
		bool m_isRunning = false;        ///< True if stream is started

		double m_measuredDepth = 0.0;
		std::unique_ptr<US::FrameGeometry> m_geometry;

		int m_previousWidth = 0;
		int m_previousHeight = 0;
		unsigned int m_previousMaskHash = 0;
		unsigned int m_lastGeometryDetectionHash = 0;
	};
}
