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

option(SI_MPI "Build with MPI support" ON)
option(SI_PYTHON_BINDINGS "Build the Python bindings" ON)
option(SI_BUILTIN_JSON  "Use the builtin version of JSON" ON)
option(SI_UNIT_TESTS "Build the C++ unit tests" ON)
option(SI_BENCHMARKS "Build benchmarks tests" OFF)


if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_VISIBILITY_INLINES_HIDDEN TRUE)  # inline symbols which dont need to be public

add_definitions(
    -Wall -Wextra -Wnon-virtual-dtor -Wunused
    -Wfatal-errors
)


#
# Dependencies
#

# Boost
set(Boost_NO_BOOST_CMAKE TRUE)  # Consistency
set(BOOST_REQ_COMPONENTS serialization filesystem)
if(SI_UNIT_TESTS)
    list(APPEND BOOST_REQ_COMPONENTS unit_test_framework)
    if(SI_BENCHMARKS)
        list(APPEND BOOST_REQ_COMPONENTS timer)
    endif()
endif()
find_package(Boost 1.59.0 REQUIRED COMPONENTS ${BOOST_REQ_COMPONENTS})
find_package(Threads REQUIRED QUIET)

# MPI
if(SI_MPI)
    find_package(MPI REQUIRED)
endif()

# JSON
if(SI_BUILTIN_JSON)
    add_subdirectory(3rdparty/nlohmann_json)
else()
    find_package(nlohmann_json REQUIRED)
endif()

#
# Target lib
#
set(SI_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)

add_library(SpatialIndex SHARED)
target_include_directories(SpatialIndex PUBLIC ${SI_INCLUDE_DIR})
target_link_libraries(SpatialIndex
  PUBLIC
    Boost::boost
    Boost::serialization
    Boost::filesystem
    Threads::Threads
    nlohmann_json::nlohmann_json
)
target_compile_definitions(SpatialIndex PUBLIC "-DBOOST_GEOMETRY_INDEX_DETAIL_EXPERIMENTAL")

if(SI_MPI)
  target_link_libraries(SpatialIndex PUBLIC MPI::MPI_CXX)
  target_compile_definitions(SpatialIndex PUBLIC "-DSI_MPI=1")
endif()

add_subdirectory(src)

#
# Py-bindings with PyBind11
#
if(SI_PYTHON_BINDINGS)
    find_package(PythonInterp REQUIRED)
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
