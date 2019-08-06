cmake_minimum_required(VERSION 3.5)

file(STRINGS "setup.py" proj_version REGEX "__version__[ ]*=")
if (proj_version MATCHES  "__version__.*'(.*)'.*")
	set(proj_version "${CMAKE_MATCH_1}")
	message(STATUS "Project: SpatialIndex ${proj_version}")
else()
	message(FATAL_ERROR "No version could be found in setup.py. Please set __version__ = 'x.y.z'")
endif()

project(SpatialIndex VERSION ${proj_version}
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


#
# Dependencies
#
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
