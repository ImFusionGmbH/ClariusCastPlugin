/* Copyright (c) 2012-2019 ImFusion GmbH, Munich, Germany. All rights reserved. */
#pragma once

#include <ImFusion/Base/AlgorithmControllerFactory.h>
#include <ImFusion/Base/AlgorithmFactory.h>
#include <ImFusion/Base/ImFusionPlugin.h>

namespace ImFusion
{
	class AlgorithmFactory;
	class AlgorithmControllerFactory;

	/// \brief	Factory for Clarius Plugin
	class ClariusAlgorithmFactory : public AlgorithmFactory
	{
	public:
		ClariusAlgorithmFactory();
	};

	/// \brief Factory for Clarius Plugin controllers
	class ClariusControllerFactory : public AlgorithmControllerFactory
	{
	public:
		ClariusControllerFactory();
		AlgorithmController* create(Algorithm* a) const override;
	};

	/// \brief Plugin for Clarius ultrasound imaging
	class ClariusPlugin : public ImFusionPlugin
	{
	public:
		ClariusPlugin();
		~ClariusPlugin() override;
		const AlgorithmFactory* getAlgorithmFactory() override;
		const AlgorithmControllerFactory* getAlgorithmControllerFactory() override;
		
		std::string pluginName() const override { return "ImFusionClarius"; }

	private:
		AlgorithmFactory* m_algFactory;
		AlgorithmControllerFactory* m_algCtrlFactory;
	};
}
