#include <cstdio>
#include <fstream>
#include <filesystem>
#include <iostream>
#define CP_SINGLE_HEADER
#include "CP.h"

#define APP_NAME "ppm"

#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_(x)

#define APP_VERSION_MAJOR 0
#define APP_VERSION_MINOR 3
#define APP_VERSION_PATCH 0

#define BUILD_INFO \
	__DATE__ " - " \
	__TIME__ 

#define APP_VERSION \
	STRINGIFY(APP_VERSION_MAJOR) "." \
	STRINGIFY(APP_VERSION_MINOR) "." \
	STRINGIFY(APP_VERSION_PATCH)

std::string replace_all(std::string& str, const std::string& from, const std::string& to) {
	if (from.empty())
		return str;
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
	}

	return str;
}

void write_file(const std::string& name, const std::string text, bool append = false, bool usePch = false)
{
	std::ofstream file;

	if (usePch)
	{
		write_file(name, "#include \"pch.h\"\n");
		append = true;
	}

	if(append)
	{
		file.open(name, std::fstream::app);
	}
	else
	{
		file.open(name);
	}

	file << text;
	file.close();
}

void ppm_write_header(const std::string& name, const std::string& architecture = "x64")
{
	std::string s = R"(workspace "__NAME__"
	startproject "__NAME__"

	architecture "__ARCHITECTURE__"

	configurations
	{
		"Debug",
		"Release",
	}

outputDir = "/%{cfg.system}/%{cfg.architecture}/%{cfg.buildcfg}")";
	s = replace_all(s, "__NAME__", name);
	s = replace_all(s, "__ARCHITECTURE__", architecture);

	write_file(name + "/premake5.lua", s);
}

void ppm_write_project(const std::string& name, const std::string& kind = "ConsoleApp", bool modifyExisting = false, const bool usePch = false, const std::string& defines = "")
{
	std::string s(R"(

project "__NAME__"
	location "__NAME__"
	kind "__KIND__"
	language "C++"
	cppdialect "C++17"
	staticruntime "On"
	characterset "Unicode"
	vectorextensions "AVX"

	targetdir ("bin/%{prj.name}" .. outputDir)
	objdir ("bin/%{prj.name}/intermediates" .. outputDir)

	__PCH__

	files
	{
		"%{prj.name}/src/include/**.h",
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	includedirs
	{
		"%{prj.name}/src/include",
	}

	defines
	{
		"__DEFINES__"
	}

	links
	{
		
	}
	
	filter "system:windows"
		systemversion "latest"

	filter "configurations:Debug"
		optimize "Debug"
		symbols "Full"

	filter "configurations:Release"
		optimize "On"
		symbols "Off")");

	const std::string pchStr =  usePch ? R"(pchheader "pch.h"
	pchsource "__NAME__/src/pch.cpp")" : "";

	s = replace_all(s, "__PCH__", pchStr);
	s = replace_all(s, "__NAME__", name);
	s = replace_all(s, "__KIND__", kind);
	s = replace_all(s, "__DEFINES__", defines);
	
	std::string lua_str = modifyExisting ? ("premake5.lua") : (name + "/premake5.lua");
	write_file(lua_str, s, true);
}

void ppm_create_default_files(const std::string& name, bool addNameToAll = true, const bool usePch = false, const std::string kind = "ConsoleApp")
{
	std::string main_path = addNameToAll ? (name + "/" + name) : name;
	if(kind == "ConsoleApp")
	{
		write_file(main_path + "/src/main.cpp", R"(#include "pch.h"

int main(int argc, char** args)
{
	printf("Hello World");
	return 0;
}
)", false, usePch);

	}

	if(kind == "WindowedApp")
	{
		// Main.cpp
		write_file(main_path + "/src/Main.cpp", R"(#include <wx/wx.h>
#include "Main.h"

cMain::cMain() 
	: wxFrame(nullptr, wxID_ANY, "Stupid Window #1", wxPoint(120, 80), wxSize(1280, 720))
{
	
}

cMain::~cMain() {}
)", false, usePch);

		// cApp.cpp
		write_file(main_path + "/src/cApp.cpp", R"(#include <wx/wx.h>
#include "Main.h"
#include "cApp.h"

wxIMPLEMENT_APP(cApp);

cApp::cApp()
{
}

cApp::~cApp()
{
	
}

bool cApp::OnInit()
{
	m_Frame = new cMain();
	m_Frame->Show();
	return true;
}

)", false, usePch);

		// cApp.h
		write_file(main_path + "/src/include/cApp.h", R"(#pragma once

class cApp : public wxApp
{
public:
	cApp();
	~cApp();

private:
	virtual bool OnInit();

	cMain* m_Frame;
};


)");

		// Main.h
		write_file(main_path + "/src/include/Main.h", R"(#pragma once

class cMain : public wxFrame
{
public:
	cMain();
	~cMain();
private:
};)");

	}
	

	if(usePch)
	{
		write_file(main_path + "/src/pch.cpp", R"(#include "pch.h")");

		write_file(main_path + "/src/pch.h", R"(#include <cstdio>
#include <string>	
#include <vector>
#include <filesystem>
#include <algorithm>
#include <chrono>
#include <utility>)");
	}	
}

void ppm_create_folders(const std::string& name, bool addNameToAll = true)
{
	if (addNameToAll)
	{
		std::filesystem::create_directory(name);
		std::filesystem::create_directory(name + "/" + name);
		std::filesystem::create_directory(name + "/" + name + "/src");
		std::filesystem::create_directory(name + "/" + name + "/src/include");
	}
	else
	{
		std::filesystem::create_directory(name);
		std::filesystem::create_directory(name + "/src");
		std::filesystem::create_directory(name + "/src/include");
	}
}

void ppm_append_project(const std::string& name, const std::string& kind = "ConsoleApp", const bool usePch = false)
{
	if (!std::filesystem::exists("./premake5.lua"))
	{
		std::cout << "Cannot append app to project " << name << ": premake5.lua doesnt exist\n";
		return;
	}
	ppm_create_folders(name, false);
	ppm_write_project(name, kind, true, usePch);

	
	ppm_create_default_files(name, false, usePch, kind);
	
}

void ppm_init_project(const std::string& name, const std::string& kind = "ConsoleApp", const bool usePch = false)
{
	ppm_create_folders(name);
	ppm_write_header(name);

	const std::string defineStr = kind == "WindowedApp" ? "WXUSINGDLL" : "";

	ppm_write_project(name, kind, false, usePch, defineStr);

	
	ppm_create_default_files(name, true, usePch, kind);
	
}

void print_help()
{
	printf("\t%-*s", 20, "<app>"); 
	printf("%s\n", "Console Application");

	printf("\t%-*s", 20, "<lib>");
	printf("%s\n", "Static Library");

	printf("\t%-*s", 20, "<dll>");
	printf("%s\n", "Dynamic Library");

	printf("\t%-*s", 20, "<win>");
	printf("%s\n", "Window Application, using WxWidget (install with vcpkg: \"vcpkg install wxwidget:x64-windows\", for further information check the vcpkg help page)");

}

int main(const int argc, char** args)
{
	auto cmdParser = CP::CommandParser(argc, args);

	cmdParser.RegisterCommand({ "-v", "Version", "Print the version" });
	cmdParser.RegisterCommand({ "-version", "Version", "Print the version" });
	cmdParser.RegisterCommand({ "-help", "Help", "Print help" });
	cmdParser.RegisterCommand({ "-pch", "PCH", "Use pch" });
	cmdParser.ConsumeFlags();
	const bool usePch = cmdParser.HasCommand("PCH");

	if (cmdParser.HasCommand("Version"))
	{
		printf("%s, v%s\n", APP_NAME, APP_VERSION);
		printf("Build Info: %s", BUILD_INFO);
		return 0;
	}

	if(cmdParser.HasCommand("Help"))
	{
		cmdParser.PrintUsage({ "init/add", "app/lib/dll/win", "name" });
		print_help();
		return 0;
	}

	if(!cmdParser.RequireParams(3))
	{
		cmdParser.PrintUsage({ "init/add", "app/lib/dll/win", "name" });
		return 1;
	}

	if (cmdParser.GetParam(1) == "init")
	{
		const auto type = cmdParser.GetParam(2);
		const auto name = cmdParser.GetParam(3);

		if (type == "app")
		{
			ppm_init_project(name, "ConsoleApp", usePch);
		}
		else if (type == "lib")
		{
			ppm_init_project(name, "StaticLib", usePch);
		}
		else if (type == "dll")
		{
			ppm_init_project(name, "SharedLib", usePch);
		}
		else if (type == "win")
		{
			ppm_init_project(name, "WindowedApp", usePch);
		}
		else
		{
			std::cout << "unsupported type given: " << type << "\n";
			return 1;
		}
		
	}
	else if (cmdParser.GetParam(1) == "add")
	{
		const auto type = cmdParser.GetParam(2);
		const auto name = cmdParser.GetParam(3);


		if (type == "app")
		{
			ppm_append_project(name);
		}
		else if (type == "lib")
		{
			ppm_append_project(name, "StaticLib", usePch);
		}
		else if (type == "dll")
		{
			ppm_append_project(name, "SharedLib", usePch);
		}
		else if (type == "win")
		{
			ppm_append_project(name, "WindowedApp", usePch);
		}
		else
		{
			std::cout << "unsupported type given: " << type << "\n";
			return 1;
		}
	}
	return 0;
}
