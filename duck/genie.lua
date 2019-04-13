--
dofile("genie_config.lua");

local PROJ_DIR = path.getdirectory(_SCRIPT)
local BUILD_DIR = path.join(PROJ_DIR, BUILD_DIR_NAME)
local GENERATED_DIR = path.join(BUILD_DIR, "src", "generated")
print(PROJ_DIR)
print(BUILD_DIR)
print(GENERATED_DIR)

local files_GENERATED_COMMON_SRC = { GENERATED_DIR.."/protocol.h", GENERATED_DIR.."/protocol.cpp", GENERATED_DIR.."/nethash.cpp" }
local files_GENERATED_SERVER_SRC = { GENERATED_DIR.."/server_data.h", GENERATED_DIR.."/server_data.cpp" }
local files_GENERATED_CLIENT_SRC = { GENERATED_DIR.."/client_data.h", GENERATED_DIR.."/client_data.cpp" }
local files_ENGINE_COMMON_SRC = { "src/base/**.h", "src/base/**.c", "src/engine/**.h", "src/engine/shared/**.cpp" }
local files_ENGINE_SERVER_CPP = { "src/engine/server/*.h", "src/engine/server/*.cpp" }
local files_ENGINE_CLIENT_CPP = { "src/engine/client/*.h", "src/engine/client/*.cpp", "src/engine/client/*.c" }
local files_GAME_COMMON_CPP = { "src/game/*.h", "src/game/*.cpp" }
local files_GAME_SERVER_CPP = { "src/game/server/**.h", "src/game/server/**.cpp" }
local files_GAME_CLIENT_CPP = { "src/game/client/**.h", "src/game/client/**.cpp", "src/game/editor/**.h", "src/game/editor/**.cpp" }

solution "Teeworlds"
	location "build_genie"
	
	configurations {
		"Debug",
		"Release"
	}

	platforms {
		"x64"
	}
	
	language "C++"
	
	configuration {"Debug"}
		flags {
			"Symbols",
			"FullSymbols"
		}
		defines {
			"_DEBUG",
			"CONF_DEBUG"
		}
		--buildoptions{ "/Z7" }
	
	configuration {"Release"}
		flags {
			"Optimize"
		}
		defines {
			"NDEBUG",
			"CONF_RELEASE"
		}
	
	configuration {}
	
	targetdir("../"..BUILD_DIR_NAME)
	
	includedirs {
		"src",
		"src/engine/external/zlib/",
		"src/engine/external/pnglite/",
		"src/engine/external/wavpack/",
        BUILD_DIR.."/src"
	}
	
	links {
        "gdi32",
        "user32",
        "ws2_32",
        "ole32",
        "shell32",
        "advapi32",
	}
	
	flags {
		"NoExceptions",
		"NoRTTI",
		"EnableSSE",
		"EnableSSE2",
		"EnableAVX",
        "EnableAVX2",
        "StaticRuntime" -- important
	}
	
	defines {
		"WIN64",
        "NO_VIZ",
        "_WIN32_WINNT=0x0501"
	}
	
	-- disable exception related warnings
	buildoptions{ "/wd4577", "/wd4530", "/wd4244", "/wd4577"}
	
project "zlib"
    kind "StaticLib"
    configuration {}
    files {"src/engine/external/zlib/*.c"}
    
project "json"
    kind "StaticLib"
    configuration {}
    files {"src/engine/external/json-parser/*.c"}
    
project "pnglite"
    kind "StaticLib"
    configuration {}
    files {"src/engine/external/pnglite/*.c"}
    
project "wavpack"
    kind "StaticLib"
    configuration {}
    files {"src/engine/external/wavpack/*.c"}
    
project "md5"
    kind "StaticLib"
    configuration {}
    files {"src/engine/external/md5/*.c"}

 project "versionsrv"
    kind "ConsoleApp"
    configuration {}
    files {files_ENGINE_COMMON_SRC, "src/versionsrv/*.cpp"}
    links { "zlib", "md5"}

project "mastersrv"
    kind "ConsoleApp"
    configuration {}
    files {files_ENGINE_COMMON_SRC, "src/mastersrv/*.cpp"}
    links { "zlib", "md5"}

project "teeworlds_srv"
	kind "ConsoleApp"
	
	configuration {}
	
	files {
		files_GENERATED_COMMON_SRC,
        files_GENERATED_SERVER_SRC,
        files_ENGINE_COMMON_SRC,
        files_ENGINE_SERVER_CPP,
        files_GAME_COMMON_CPP,
        files_GAME_SERVER_CPP
	}
    
    links {
        "zlib",
        "md5"
    }
    
    configuration {"Debug"}
    targetsuffix "_dbg"
    
project "teeworlds"
	kind "ConsoleApp"
	
	configuration {}
	
	files {
		files_GENERATED_COMMON_SRC,
        files_GENERATED_CLIENT_SRC,
        files_ENGINE_COMMON_SRC,
        files_ENGINE_CLIENT_CPP,
        files_GAME_COMMON_CPP,
        files_GAME_CLIENT_CPP
	}
    
    includedirs {
		SDL2_include,
        freetype_include,
        curl_include,
        zip_include,
	}
    
    links {
        "opengl32",
        "glu32",
        "winmm",
        
        "zlib",
        "json",
        "pnglite",
        "wavpack",
        "md5",
        
        "SDL2",
        "freetype",
        "libcurl",
        "zip",
    }
    
    libdirs {
        SDL2_libdir,
        freetype_libdir,
        curl_libdir,
        zip_libdir,
    }
    
    configuration {"Debug"}
    targetsuffix "_dbg"