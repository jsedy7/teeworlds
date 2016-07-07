project(Teeworlds)
cmake_minimum_required(VERSION 3.0.0) # find freetype 2
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

### Utils
set(COMP_FLAGS "")
function(add_compile_flag flag)
	set(COMP_FLAGS "${COMP_FLAGS} ${flag}" PARENT_SCOPE)
endfunction()

function(add_define def)
if(MSVC)
	set(COMP_FLAGS "${COMP_FLAGS} /D${def}" PARENT_SCOPE)
else()
	set(COMP_FLAGS "${COMP_FLAGS} -D${def}" PARENT_SCOPE)
endif()
endfunction()

function(content_compile action output)
	execute_process(
		COMMAND "python" "${CMAKE_SOURCE_DIR}/datasrc/compile.py" "${action}"
		OUTPUT_FILE ${output}
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	)
endfunction()

function(chash output)
	execute_process(
		COMMAND "python" "${CMAKE_SOURCE_DIR}/scripts/cmd5.py" ${ARGN}
		OUTPUT_FILE ${output}
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	)
endfunction()

### Copy content
if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/data")
	message(STATUS "Copying content...")
	file(COPY "datasrc" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}" # copy datasrc
		FILES_MATCHING
		PATTERN "*.png"
		PATTERN "*.wv"
		PATTERN "*.ttf"
		PATTERN "*.txt"
		PATTERN "*.map"
		PATTERN "*.json"
	)
	file(RENAME "${CMAKE_CURRENT_BINARY_DIR}/datasrc" "${CMAKE_CURRENT_BINARY_DIR}/data") # rename it "data"
	message(STATUS "Content copied.")
endif()

### File generation
set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/src/generated")
file(MAKE_DIRECTORY ${GENERATED_DIR}) # create the "generated" directory
content_compile("network_source" "${GENERATED_DIR}/protocol.cpp")
content_compile("network_header" "${GENERATED_DIR}/protocol.h")
content_compile("server_content_source" "${GENERATED_DIR}/server_data.cpp")
content_compile("server_content_header" "${GENERATED_DIR}/server_data.h")
content_compile("client_content_source" "${GENERATED_DIR}/client_data.cpp")
content_compile("client_content_header" "${GENERATED_DIR}/client_data.h")
chash("${GENERATED_DIR}/nethash.cpp" "src/engine/shared/protocol.h" "src/game/tuning.h" "src/game/gamecore.cpp" "${GENERATED_DIR}/protocol.h")
message(STATUS "Files generated (${GENERATED_DIR})")

if(NOT MSVC)
	add_compile_flag("-m64") # x64
	add_compile_flag("-Wall -fno-exceptions")
endif()

add_define("NO_VIZ")
add_define("_WIN32_WINNT=0x0501")

if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
	add_define("CONF_RELEASE")
	message(STATUS "CONF_RELEASE")
	if(MSVC)
		set(CMAKE_CXX_FLAGS_RELEASE "/O2 /DNDEBUG")
	else()
		set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
	endif()
else()
	add_define("CONF_DEBUG")
	message(STATUS "CONF_DEBUG")
endif()

### Engine libs
include_directories("src/engine/external/zlib/")

file(GLOB ZLIB_C "src/engine/external/zlib/*.c")
file(GLOB JSON_C "src/engine/external/json-parser/*.c")
file(GLOB PNGLITE_C "src/engine/external/pnglite/*.c")
file(GLOB WAVPACK_C "src/engine/external/wavpack/*.c")
file(GLOB MD5_C "src/engine/external/md5/*.c")

add_library(zlib STATIC ${ZLIB_C})
add_library(json STATIC ${JSON_C})
add_library(pnglite STATIC ${PNGLITE_C})
add_library(wavpack STATIC ${WAVPACK_C})
add_library(md5 STATIC ${MD5_C})

set(COMMON_LIBS gdi32 user32 ws2_32 ole32 shell32 advapi32 imm32 version oleaut32)
# Don't link unused libs
set(CMAKE_CXX_STANDARD_LIBRARIES "")
set(CMAKE_C_STANDARD_LIBRARIES "")
# Apply flags
set(CMAKE_CXX_FLAGS ${COMP_FLAGS})
set(CMAKE_C_FLAGS ${COMP_FLAGS})
message(STATUS "flags:${COMP_FLAGS}")

### Source files
include_directories("src")
include_directories("${CMAKE_CURRENT_BINARY_DIR}/src") # generated files

file(GLOB GENERATED_COMMON_SRC "${GENERATED_DIR}/protocol.h" "${GENERATED_DIR}/protocol.cpp" "${GENERATED_DIR}/nethash.cpp")
file(GLOB GENERATED_SERVER_SRC "${GENERATED_DIR}/server_data.h" "${GENERATED_DIR}/server_data.cpp")
file(GLOB GENERATED_CLIENT_SRC "${GENERATED_DIR}/client_data.h" "${GENERATED_DIR}/client_data.cpp")
file(GLOB_RECURSE ENGINE_COMMON_SRC "src/base/*.h" "src/base/*.c" "src/engine/*.h" "src/engine/shared/*.cpp")
file(GLOB ENGINE_SERVER_CPP "src/engine/server/*.h" "src/engine/server/*.cpp")
file(GLOB ENGINE_CLIENT_CPP "src/engine/client/*.h" "src/engine/client/*.cpp")
file(GLOB GAME_COMMON_CPP "src/game/*.h" "src/game/*.cpp")
file(GLOB_RECURSE GAME_SERVER_CPP "src/game/server/*.h" "src/game/server/*.cpp")
file(GLOB_RECURSE GAME_CLIENT_CPP "src/game/client/*.h" "src/game/client/*.cpp" "src/game/editor/*.h" "src/game/editor/*.cpp")
set(SERVER_SRC ${GENERATED_COMMON_SRC} ${GENERATED_SERVER_SRC} ${ENGINE_COMMON_SRC} ${ENGINE_SERVER_CPP} ${GAME_COMMON_CPP} ${GAME_SERVER_CPP})
set(CLIENT_SRC ${GENERATED_COMMON_SRC} ${GENERATED_CLIENT_SRC} ${ENGINE_COMMON_SRC} ${ENGINE_CLIENT_CPP} ${GAME_COMMON_CPP} ${GAME_CLIENT_CPP})

### Tools
file(GLOB TOOLS_FILES "src/tools/*.c" "src/tools/*.cpp")
foreach(_f ${TOOLS_FILES})
	get_filename_component(toolName ${_f} NAME_WE)
	add_executable(${toolName} ${ENGINE_COMMON_SRC} ${_f})
	target_link_libraries(${toolName} ${COMMON_LIBS})
	target_link_libraries(${toolName} zlib wavpack pnglite md5)
endforeach()

### Master and Version servers
set(MASTER_SRV_EXE_NAME "mastersrv")
set(VERSION_SRV_EXE_NAME "versionsrv")
file(GLOB MASTER_SRV_SRC "src/mastersrv/*.cpp")
file(GLOB VERSION_SRV_SRC "src/versionsrv/*.cpp")
add_executable(${MASTER_SRV_EXE_NAME} ${ENGINE_COMMON_SRC} ${MASTER_SRV_SRC})
target_link_libraries(${MASTER_SRV_EXE_NAME} ${COMMON_LIBS})
target_link_libraries(${MASTER_SRV_EXE_NAME} zlib md5)
add_executable(${VERSION_SRV_EXE_NAME} ${ENGINE_COMMON_SRC} ${VERSION_SRV_SRC})
target_link_libraries(${VERSION_SRV_EXE_NAME} ${COMMON_LIBS})
target_link_libraries(${VERSION_SRV_EXE_NAME} zlib md5)

### Client and Server
set(CLIENT_EXE_NAME "teeworlds")
set(SERVER_EXE_NAME "teeworlds_srv")

add_executable(${SERVER_EXE_NAME} ${SERVER_SRC} "other/icons/teeworlds_srv_gcc.rc")
target_link_libraries(${SERVER_EXE_NAME} ${COMMON_LIBS})
target_link_libraries(${SERVER_EXE_NAME} zlib md5)

add_executable(${CLIENT_EXE_NAME} ${CLIENT_SRC} "other/icons/teeworlds_gcc.rc")
target_link_libraries(${CLIENT_EXE_NAME} ${COMMON_LIBS} opengl32 glu32 winmm)
target_link_libraries(${CLIENT_EXE_NAME} zlib json pnglite wavpack md5)

# Find freetype2
find_package(freetype REQUIRED)
if(${FREETYPE_FOUND})
	include_directories(${FREETYPE_INCLUDE_DIRS})
	target_link_libraries(${CLIENT_EXE_NAME} ${FREETYPE_LIBRARIES})
endif()

# Find SDL2
find_package(SDL2 REQUIRED)
if(${SDL2_FOUND})
	include_directories(${SDL2_INCLUDE_DIR})
	target_link_libraries(${CLIENT_EXE_NAME} ${SDL2_LIBRARY})
endif()
