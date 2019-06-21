cmake_minimum_required(VERSION 3.5)

project(SpatialIndex VERSION 0.0.0
					 DESCRIPTION "Spatial indexing tools for all the family"
					 LANGUAGES CXX)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VERBOSE_MAKEFILE ON)

option(PYTHON_BINDINGS "Build the python bindings" ON)

include_directories(/usr/local/include/)

find_package(Boost 1.65.0 REQUIRED COMPONENTS serialization)

set(si_include_directory ${PROJECT_SOURCE_DIR}/include)

add_definitions("-std=c++14")

add_library(SpatialIndex INTERFACE)
target_include_directories(SpatialIndex INTERFACE ${si_include_directory})

add_subdirectory(tests)


if(PYTHON_BINDINGS)
	add_subdirectory(3rdparty/pybind11)
	add_subdirectory(python)
endif()

