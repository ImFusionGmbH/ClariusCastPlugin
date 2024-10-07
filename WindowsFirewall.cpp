#include "WindowsFirewall.h"

#include <windows.h>
#include <array>
#include <memory>
#include <sstream>

namespace
{
	// from https://stackoverflow.com/questions/1528298/get-path-of-executable
	std::string get_executable_full_path()
	{
		std::string path = "";
		HMODULE hModule = GetModuleHandle(NULL); // exe itself
		if (hModule != NULL)
		{
			char ownPth[MAX_PATH];
			GetModuleFileName(hModule, ownPth, (sizeof(ownPth)));
			path = std::string(ownPth);
		}
		return path;
	}


	// from https://stackoverflow.com/questions/478898/how-to-execute-a-command-and-get-output-of-command-within-c-using-posix
	std::string exec_command(const char* cmd)
	{
		std::array<char, 128> buffer;
		std::string result;
		std::shared_ptr<FILE> pipe(_popen(cmd, "r"), _pclose);
		if (!pipe) return "";
		while (!feof(pipe.get())) {
			if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
				result += buffer.data();
		}
		return result;
	}
}


namespace ImFusion
{
	bool WindowsFirewall::checkFirewallRuleExists()
	{
		// make sure there is run with the right name and the correct executable (will be different depending on repo vs. installed)
		std::string outp = exec_command("netsh advfirewall firewall show rule name=ClariusImFusion dir=in verbose");
		std::string exepath = get_executable_full_path();
		return (outp.find("ClariusImFusion") != std::string::npos && outp.find(exepath) != std::string::npos);
	}


	void WindowsFirewall::addFirewallRule()
	{
		std::string s;
		s = "/C netsh advfirewall firewall delete rule name=ClariusImFusion & netsh advfirewall firewall add rule name=ClariusImFusion dir=in action=allow protocol=UDP program=\"";
		s += get_executable_full_path();
		s += "\" localport=32768-65535";

		// Run elevated and block with process is running
		SHELLEXECUTEINFO shExecInfo = { 0 };
		shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
		shExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
		shExecInfo.hwnd = NULL;
		shExecInfo.lpVerb = "runas";
		shExecInfo.lpFile = "cmd";
		shExecInfo.lpParameters = s.c_str(); // ss.str().c_str();
		shExecInfo.lpDirectory = NULL;
		shExecInfo.nShow = SW_HIDE;
		shExecInfo.hInstApp = NULL;
		ShellExecuteEx(&shExecInfo);
		WaitForSingleObject(shExecInfo.hProcess, 3000);
		CloseHandle(shExecInfo.hProcess);
	}
}