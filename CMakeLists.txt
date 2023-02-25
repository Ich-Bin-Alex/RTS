cmake_minimum_required(VERSION 3.11)
set(CMAKE_C_COMPILER gcc)

project(rts)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Dependencies
set(RAYLIB_VERSION 4.2.0)
find_package(raylib ${RAYLIB_VERSION} QUIET) # QUIET or REQUIRED
if (NOT raylib_FOUND) # If there's none, fetch and build raylib
	include(FetchContent)
	FetchContent_Declare(
		raylib
		URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
	)
	FetchContent_GetProperties(raylib)
	if (NOT raylib_POPULATED) # Have we downloaded raylib yet?
		set(FETCHCONTENT_QUIET NO)
		FetchContent_Populate(raylib)
		set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # don't build the supplied examples
		add_subdirectory(${raylib_SOURCE_DIR} ${raylib_BINARY_DIR})
	endif()
endif()

add_compile_options(-Wall -g -Wno-unused-function -Wno-missing-braces -Wno-discarded-qualifiers)
#add_link_options(-fsanitize=address,undefined,leak)

include_directories(. source/tools)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/..)

add_executable(rts 
	rts.c 
	source/tools/helper.c
	source/game.c
	source/map.c
	source/unit.c
	source/building.c
	source/interface.c)

target_link_libraries(rts raylib)