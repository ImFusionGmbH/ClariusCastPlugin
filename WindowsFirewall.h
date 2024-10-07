/* Copyright (c) 2012-2019 ImFusion GmbH, Munich, Germany. All rights reserved. */
#pragma once

namespace ImFusion
{
	/**	\brief	Helper class to programmatically set firewall exception rule under Windows
	 *	\author	Oliver Zettinig
	 */
	class WindowsFirewall
	{
	public:
		/// Checks if a suitable firewall rule exists
		static bool checkFirewallRuleExists();

		/// Adds a firewall exception to allow the ImFusionSuite executable incoming UDP connections on ports >32768
		static void addFirewallRule();
	};
}
