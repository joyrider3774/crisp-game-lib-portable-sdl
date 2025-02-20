cmake_minimum_required(VERSION 3.5)

project(CglpSDL3 LANGUAGES C)

# Create an option to switch between a system sdl library and a vendored sdl library
option(USE_VENDORED_SDL "Use vendored libraries" OFF)

if(USE_VENDORED_SDL)
	set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
	set(SDL_STATIC_PIC ON CACHE BOOL "" FORCE)
	add_subdirectory("${PROJECT_SOURCE_DIR}/../../SDL3" "${PROJECT_SOURCE_DIR}/SDL3" EXCLUDE_FROM_ALL)
else()
    # 1. Look for a SDL3 package, 2. look for the SDL3 component and 3. fail if none can be found
    find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3)
endif()


file(GLOB SRC CONFIGURE_DEPENDS
        "${PROJECT_SOURCE_DIR}/*.c"
        "${PROJECT_SOURCE_DIR}/*.h"
        "${PROJECT_SOURCE_DIR}/../games/*.c"
        "${PROJECT_SOURCE_DIR}/../games/*.h"
        "${PROJECT_SOURCE_DIR}/../lib/*.c"
        "${PROJECT_SOURCE_DIR}/../lib/*.h"
)

if (WIN32)
	set(CMAKE_EXE_LINKER_FLAGS "-static -mwindows")
endif()

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE ${SRC} )

target_include_directories(${PROJECT_NAME} PRIVATE 
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/../games"
    "${PROJECT_SOURCE_DIR}/../lib"
)

if(USE_VENDORED_SDL)
    target_link_libraries(${PROJECT_NAME} PRIVATE 
        SDL3::SDL3-static
    )
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE 
        SDL3::SDL3
    )
endif()