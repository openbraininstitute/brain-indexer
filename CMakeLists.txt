cmake_minimum_required(VERSION 3.5)

include("CMake/version.cmake")
if(NOT DEFINED SI_VERSION)
    get_git_version(SI_VERSION)
endif()
sanitize_version(SI_VERSION "${SI_VERSION}")
message(STATUS "Building Spatial-Index ${SI_VERSION}")

project(SpatialIndex VERSION ${SI_VERSION}
                     DESCRIPTION "BBP Spatial indexing tools"
                     LANGUAGES CXX)

option(SI_PYTHON_BINDINGS "Build the python bindings" ON)
option(SI_UNIT_TESTS "Build the c++ unit tests" ON)
option(SI_BENCHMARKS "Build benchmarks tests" OFF)


if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_definitions(-Wall -Wfatal-errors)


#
# Dependencies
#
set(Boost_NO_BOOST_CMAKE TRUE)  # Consistency
set(BOOST_REQ_COMPONENTS serialization)
if(SI_UNIT_TESTS)
    list(APPEND BOOST_REQ_COMPONENTS unit_test_framework)
    if(SI_BENCHMARKS)
        list(APPEND BOOST_REQ_COMPONENTS timer)
    endif()
endif()
find_package(Boost 1.65.0 REQUIRED COMPONENTS ${BOOST_REQ_COMPONENTS})


#
# Target lib
#
set(SI_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)

add_library(SpatialIndex INTERFACE)
target_include_directories(SpatialIndex INTERFACE ${SI_INCLUDE_DIR})
target_link_libraries(SpatialIndex INTERFACE Boost::boost Boost::serialization)
target_compile_definitions(SpatialIndex INTERFACE "-DBOOST_GEOMETRY_INDEX_DETAIL_EXPERIMENTAL")


#
# Py-bindings with PyBind11
#
if(SI_PYTHON_BINDINGS)
    find_package(PythonInterp)
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
