cmake_minimum_required(VERSION 3.20)

if(DEFINED ENV{VCPKG_DEFAULT_TRIPLET} AND NOT DEFINED VCPKG_TARGET_TRIPLET)
  set(VCPKG_TARGET_TRIPLET "$ENV{VCPKG_DEFAULT_TRIPLET}" CACHE STRING "The vcpkg triplet")
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(MACOSX TRUE)
endif()

project(CglpSDL2 C LINKER_LANGUAGE C)

find_package(SDL2 CONFIG REQUIRED)

file(GLOB SRC CONFIGURE_DEPENDS
        "${PROJECT_SOURCE_DIR}/src/cglpSDL2"
        "${PROJECT_SOURCE_DIR}/src/cglpSDL2/*.h"
        "${PROJECT_SOURCE_DIR}/src/games/*.cpp"
        "${PROJECT_SOURCE_DIR}/src/games/*.h"
		"${PROJECT_SOURCE_DIR}/src/lib/*.cpp"
        "${PROJECT_SOURCE_DIR}/src/lib/*.h"
        )

set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
set(BUILD_SHARED_LIBS OFF)
if (! ${MACOSX})
set(CMAKE_EXE_LINKER_FLAGS "-static")
endif()

add_executable(CglpSDL2)
target_sources(${PROJECT_NAME} PRIVATE ${SRC} )
if (! ${MACOSX})
target_link_libraries(${PROJECT_NAME} PRIVATE -static-libgcc)
endif()
target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2main SDL2::SDL2-static)
