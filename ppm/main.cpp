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
#define APP_VERSION_MINOR 2
#define APP_VERSION_PATCH 2

#define BUILD_INFO \
	STRINGIFY(__DATE__) " - " \
	STRINGIFY(__TIME__)

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

void ppm_write_header(const std::string& name, const char* architecture = "x64")
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
	std::ofstream file(name + "/premake5.lua");
	file.write(s.c_str(), s.size());
	file.close();

}

void ppm_write_project(const std::string& name, const char* kind = "ConsoleApp", bool modifyExisting = false)
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

	pchheader "pch.h"
	pchsource "__NAME__/src/pch.cpp"

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
	s = replace_all(s, "__NAME__", name);
	s = replace_all(s, "__KIND__", kind);
	std::string lua_str = modifyExisting ? ("premake5.lua") : (name + "/premake5.lua");
	std::ofstream file(lua_str, std::fstream::app);
	file.write(s.c_str(), s.size());
	file.close();
}

void ppm_create_default_files(const std::string& name, bool addNameToAll = true)
{
	std::string main_path = addNameToAll ? (name + "/" + name) : name;
	std::ofstream temp(main_path + "/src/main.cpp");
	std::string s(R"(#include "pch.h"

int main(int argc, char** args)
{
	printf("Hello World");
	return 0;
}
)");
	temp.write(s.c_str(), s.size());
	temp.close();

	temp.open(main_path + "/src/pch.cpp");
	s = R"(#include "pch.h")";
	temp.write(s.c_str(), s.size());
	temp.close();

	temp.open(main_path + "/src/pch.h");
	s = R"(#include <cstdio>
#include <string>	
#include <vector>
#include <filesystem>
#include <algorithm>
#include <chrono>
#include <utility>)";
	temp.write(s.c_str(), s.size());
	temp.close();
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

void ppm_append_project(const char* name, const char* kind = "ConsoleApp", const char* architecture = "x64")
{
	if (!std::filesystem::exists("./premake5.lua"))
	{
		std::cout << "Cannot append app to project " << name << ": premake5.lua doesnt exist\n";
		return;
	}
	ppm_create_folders(name, false);
	ppm_write_project(name, kind, true);

	if(!strcmp(kind, "ConsoleApp"))
	{
		ppm_create_default_files(name, false);
	}
}

void ppm_init_project(const char* name, const char* kind = "ConsoleApp", const char* architecture = "x64")
{
	ppm_create_folders(name);
	ppm_write_header(name);
	ppm_write_project(name, kind);

	if (!strcmp(kind, "ConsoleApp"))
	{
		ppm_create_default_files(name);
	}
}

int main(const int argc, char** args)
{
	auto cmdParser = new CP::CommandParser(argc, args);

	cmdParser->RegisterCommand({ "-v", "Version", "Print the version" });
	cmdParser->ConsumeFlags();

	if(!cmdParser->RequireParams(3))
	{
		cmdParser->PrintUsage({ "init/add", "app/lib/dll/win", "name" });
		return 1;
	}

	if (cmdParser->HasCommand("Version"))
	{
		printf("%s, v%s\n", APP_NAME, APP_VERSION);
		printf("Build Info: %s", BUILD_INFO);
		return 0;
	}


	if (cmdParser->GetParam(1) == "init")
	{
		const auto type = cmdParser->GetParam(2);
		const auto name = cmdParser->GetParam(3).c_str();
		ppm_init_project(name);


		if (type == "app")
		{
			ppm_init_project(name);
		}
		else if (type == "lib")
		{
			ppm_init_project(name, "StaticLib");
		}
		else if (type == "dll")
		{
			ppm_init_project(name, "SharedLib");
		}
		else if (type == "win")
		{
			ppm_init_project(name, "WindowedApp");
		}
		else
		{
			std::cout << "unsupported type given: " << type << "\n";
			return 1;
		}
		
	}
	else if (cmdParser->GetParam(1) == "add")
	{
		const auto type = cmdParser->GetParam(2);
		const auto name = cmdParser->GetParam(3).c_str();
		ppm_init_project(name);


		if (type == "app")
		{
			ppm_append_project(name);
		}
		else if (type == "lib")
		{
			ppm_append_project(name, "StaticLib");
		}
		else if (type == "dll")
		{
			ppm_append_project(name, "SharedLib");
		}
		else if (type == "win")
		{
			ppm_append_project(name, "WindowedApp");
		}
		else
		{
			std::cout << "unsupported type given: " << type << "\n";
			return 1;
		}
	}
	return 0;
}
