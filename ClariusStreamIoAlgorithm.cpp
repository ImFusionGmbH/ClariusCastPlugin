#include "ClariusStreamIoAlgorithm.h"

#include "ClariusStream.h"

#include <ImFusion/Core/Log.h>

#ifdef WIN32
#	include "WindowsFirewall.h"
#endif

namespace ImFusion
{
	ClariusStreamIoAlgorithm::ClariusStreamIoAlgorithm()
		: CreateStreamIoAlgorithm<ClariusStream, false, false>() {
	}

	/// Returns true if data is empty. If a is not 0, create algorithm with input data.
	bool ClariusStreamIoAlgorithm::createCompatible(const DataList& data, Algorithm** a)
	{
		if (data.size())
			return false;
		if (a)
			*a = new ClariusStreamIoAlgorithm();
		return true;
	}

	void ClariusStreamIoAlgorithm::compute()
	{
		if (!ClariusStream::canInstantiate())
		{
			LOG_ERROR("Only one ClariusStream instance allowed at a time!");
			return;
		}

#ifdef WIN32
		if (!WindowsFirewall::checkFirewallRuleExists())
		{
			LOG_INFO("No firewall exception detected for Clarius streaming. Trying to add one...");
			WindowsFirewall::addFirewallRule();
			if (WindowsFirewall::checkFirewallRuleExists())
				LOG_INFO("Firewall exception added.");
			else
				LOG_ERROR("Firewall exception could not be added. Please make sure that incoming UDP connections are allowed.");
		}
#endif

		CreateStreamIoAlgorithm<ClariusStream, false, false>::compute();

		if (m_fail || m_stream->p_serverAddress.value().empty())
			return;    // open needs to be called later when IP is known

		if (m_fail)
		{
			m_stream = nullptr;
			return;
		}
	}
}
