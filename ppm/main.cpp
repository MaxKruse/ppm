#include <cstdio>
#include <fstream>
#include <filesystem>
#include <iostream>

#define APP_NAME "ppm"

#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_(x)

#define APP_VERSION_MAJOR 0
#define APP_VERSION_MINOR 1
#define APP_VERSION_PATCH 0

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

void ppm_write_header(std::string name, const char* architecture = "x64")
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

void ppm_write_project(std::string name, bool modifyExisting = false, const char* architecture = "x64", const char* kind = "ConsoleApp")
{
	std::string s(R"(

project "__NAME__"
	location "__NAME__"
	kind "__KIND__"
	language "C++"
	cppdialect "C++17"
	staticruntime "On"

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
		"%{prj.name}/include",
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

	std::string lua_str;
	if(modifyExisting)
	{
		lua_str = "premake5.lua";
	}
	else
	{
		lua_str = name + "/premake5.lua";
	}
	std::ofstream file(lua_str, std::fstream::app);
	file.write(s.c_str(), s.size());
	file.close();
}

void ppm_create_default_files(const std::string name)
{
	std::ofstream temp(name + "/"+ name + "/src/main.cpp");
	std::string s(R"(#include "pch.h"

int main(int argc, char** args)
{
	printf("Hello World");
	return 0;
}
)");
	temp.write(s.c_str(), s.size());
	temp.close();

	temp.open(name + "/" + name + "/src/pch.cpp");
	s = R"(#include "pch.h")";
	temp.write(s.c_str(), s.size());
	temp.close();

	temp.open(name + "/" + name + "/src/pch.h");
	s = R"(#include <cstdio>
#include <string>	
#include <vector>
#include <filesystem>
#include <algorithm>
#include <chrono>)";
	temp.write(s.c_str(), s.size());
	temp.close();
}

void ppm_create_folders(const std::string name, bool addNameToAll = true)
{
	if(addNameToAll)
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

void ppm_append_app(const std::string name, const char* architecture = "x64")
{
	if (!std::filesystem::exists("/premake5.lua"))
	{
		std::cerr << "cannot append app to project " << name << ": premake5.lua doesnt exist";
		return;
	}
	ppm_create_folders(name, false);
	ppm_write_project(name, true);
}

void ppm_append_lib(const std::string name, const char* architecture = "x64")
{
	if (!std::filesystem::exists("/premake5.lua"))
	{
		std::cerr << "cannot append lib to project " << name << ": premake5.lua doesnt exist";
		return;
	}
	ppm_create_folders(name, false);
	ppm_write_project(name, true);
}

void ppm_init_app(const char* name, const char* architecture = "x64")
{
	ppm_create_folders(name);
	ppm_write_header(name);
	ppm_write_project(name);
	ppm_create_default_files(name);
}

void ppm_init_lib(const char* name, const char* architecture = "x64")
{
	ppm_create_folders(name);
	ppm_write_header(name);
	ppm_write_project(name, "StaticLib");
	ppm_create_default_files(name);
}

void print_usage()
{
	printf("Usage: %s init [app/lib] <name>\n", APP_NAME);
	printf("Usage: %s add <app/lib> <name>\n", APP_NAME);
	printf("Usage: %s <version/-v/-version>\n", APP_NAME);
}

int main(int argc, char** args)
{
	srand(static_cast<unsigned>(time(nullptr)));
	if(argc == 2)
	{
		if(!strcmp(args[1], "version") || !strcmp(args[1], "-v") || !strcmp(args[1], "-version"))
		{
			printf("%s, v%s", APP_NAME, APP_VERSION);
			return 0;
		}		
	}

	if (argc != 3 && argc != 4)
	{
		print_usage();
		return 1;
	}

	const auto cmd = args[1];

	if(!strcmp(cmd, "init"))
	{
		char* name;

		if (argc == 3)
		{
			name = args[2];
			ppm_init_app(name);
		}

		if (argc == 4)
		{
			name = args[3];
			const auto type = args[2];

			if (!strcmp(type, "lib"))
			{
				ppm_init_lib(name);
			}
			else
			{
				ppm_init_app(name);
			}
		}
	}
	else if(!strcmp(cmd, "add"))
	{
		char* name;

		if (argc == 4)
		{
			name = args[3];
			const auto type = args[2];

			if (!strcmp(type, "lib"))
			{
				ppm_append_lib(name);
			}
			else
			{
				ppm_append_app(name);
			}
		}
	}
	else
	{
		print_usage();
		return 1;
	}
	return 0;
}