#include "ClariusPlugin.h"

#include "ClariusController.h"
#include "ClariusStreamIoAlgorithm.h"

#include <ImFusion/Base/DataComponentFactory.h>

#ifdef WIN32
extern "C" __declspec(dllexport) ImFusion::ImFusionPlugin* createPlugin()
#else
extern "C" ImFusion::ImFusionPlugin* createPlugin()
#endif
{
	return new ImFusion::ClariusPlugin;
}

namespace ImFusion
{
	ClariusAlgorithmFactory::ClariusAlgorithmFactory()
		: AlgorithmFactory("ClariusCast")
	{
		registerAlgorithm<ClariusStreamIoAlgorithm>("ClariusStreamIo", "IO;Clarius Stream");
	}

	ClariusControllerFactory::ClariusControllerFactory()
		: AlgorithmControllerFactory("ClariusCast"){};

	AlgorithmController* ClariusControllerFactory::create(Algorithm* a) const
	{
		if (auto* alg = dynamic_cast<ClariusStreamIoAlgorithm*>(a))
			return new ClariusController(alg);

		return nullptr;
	}


	ClariusPlugin::ClariusPlugin()
		: m_algFactory(new ClariusAlgorithmFactory)
		, m_algCtrlFactory(new ClariusControllerFactory)
	{
	}


	ClariusPlugin::~ClariusPlugin() {}


	const ImFusion::AlgorithmFactory* ClariusPlugin::getAlgorithmFactory() { return m_algFactory; }


	const ImFusion::AlgorithmControllerFactory* ClariusPlugin::getAlgorithmControllerFactory() { return m_algCtrlFactory; }
}
