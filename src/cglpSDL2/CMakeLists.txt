cmake_minimum_required(VERSION 3.20)

project(CglpSDL2 LANGUAGES C)

# Create an option to switch between a system sdl library and a vendored sdl library
option(USE_VENDORED_SDL "Use vendored libraries" OFF)

if(USE_VENDORED_SDL)
    add_subdirectory("${PROJECT_SOURCE_DIR}/../../SDL2" "${PROJECT_SOURCE_DIR}/SDL2" EXCLUDE_FROM_ALL)
else()
    # 1. Look for a SDL2 package, 2. look for the SDL2 component and 3. fail if none can be found
    find_package(SDL2 REQUIRED CONFIG REQUIRED COMPONENTS SDL2)

    # 1. Look for a SDL2 package, 2. Look for the SDL2maincomponent and 3. DO NOT fail when SDL2main is not available
    find_package(SDL2 REQUIRED CONFIG COMPONENTS SDL2main)
endif()


file(GLOB SRC CONFIGURE_DEPENDS
        "${PROJECT_SOURCE_DIR}/*.c"
        "${PROJECT_SOURCE_DIR}/*.h"
        "${PROJECT_SOURCE_DIR}/../games/*.c"
        "${PROJECT_SOURCE_DIR}/../games/*.h"
		"${PROJECT_SOURCE_DIR}/../lib/*.c"
        "${PROJECT_SOURCE_DIR}/../lib/*.h"
        )

set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
set(BUILD_SHARED_LIBS OFF)
if (!(${CMAKE_SYSTEM_NAME} MATCHES "Darwin"))
	set(CMAKE_EXE_LINKER_FLAGS "-static")
endif()

if (WIN32)
set(CMAKE_EXE_LINKER_FLAGS "-mwindows")
endif()

add_executable(CglpSDL2)
target_sources(${PROJECT_NAME} PRIVATE ${SRC} )
target_include_directories(${PROJECT_NAME} PRIVATE "${PROJECT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/../games" "${PROJECT_SOURCE_DIR}/../lib")
if(TARGET SDL2::SDL2main)
	target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2main)
endif()
target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2-static)
target_link_libraries(${PROJECT_NAME} PRIVATE m)
