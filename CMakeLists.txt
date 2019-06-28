cmake_minimum_required(VERSION 3.2)

project(SpatialIndex VERSION 0.0.1
                     DESCRIPTION "BBP Spatial indexing tools"
                     LANGUAGES CXX)

option(SI_PYTHON_BINDINGS "Build the python bindings" ON)
option(SI_UNIT_TESTS "Build the c++ unit tests" ON)


if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


include_directories("/usr/local/include/")


#
# Dependencies
#
set(BOOST_REQ_COMPONENTS serialization)
if(SI_UNIT_TESTS)
    set(BOOST_REQ_COMPONENTS ${BOOST_REQ_COMPONENTS} unit_test_framework)
endif()
find_package(Boost 1.65.0 REQUIRED COMPONENTS ${BOOST_REQ_COMPONENTS})


#
# Target lib
#
set(si_include_directory ${PROJECT_SOURCE_DIR}/include)

add_library(SpatialIndex INTERFACE)
target_include_directories(SpatialIndex INTERFACE ${si_include_directory})


#
# Py-bindings with PyBind11
#
if(SI_PYTHON_BINDINGS)
    add_subdirectory(3rdparty/pybind11)
    add_subdirectory(python)
endif()


#
# Tests
#
if(SI_UNIT_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
