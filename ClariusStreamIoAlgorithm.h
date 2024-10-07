/* Copyright (c) 2012-2019 ImFusion GmbH, Munich, Germany. All rights reserved. */
#pragma once

#include <ImFusion/Stream/CreateStreamIoAlgorithm.h>
#include <ImFusion/Stream/ImageStream.h>

#include <memory>

namespace ImFusion
{
	class ClariusStream;

	/** \brief	IO Algorithm for creating a Clarius ultrasound stream
	 *	\author	Oliver Zettinig
	 */
	class ClariusStreamIoAlgorithm : public CreateStreamIoAlgorithm<ClariusStream, false, false>
	{
	public:
		ClariusStreamIoAlgorithm();

		static bool createCompatible(const DataList& data, Algorithm** a = nullptr);

		void compute() override;
	};
}
