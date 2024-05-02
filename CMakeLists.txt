cmake_minimum_required(VERSION 3.5)

project(BrainIndexer DESCRIPTION "BBP Spatial indexing tools"
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
find_package(Boost 1.79.0 REQUIRED COMPONENTS ${BOOST_REQ_COMPONENTS})
find_package(Threads REQUIRED QUIET)

# MPI
if(SI_MPI)
    find_package(MPI REQUIRED)
    include(CMake/mpi_launcher.cmake)
endif()

# JSON
if(SI_BUILTIN_JSON)
    add_subdirectory(3rdparty/nlohmann_json)
else()
    find_package(nlohmann_json REQUIRED)
endif()

set(ZisaSFC_BUILD_TESTS Off CACHE BOOL "Should ZisaSFC tests be built?")
add_subdirectory(3rdparty/zisa.sfc EXCLUDE_FROM_ALL)

#
# Target lib
#
set(SI_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)

add_library(BrainIndexer INTERFACE)
target_include_directories(BrainIndexer INTERFACE ${SI_INCLUDE_DIR})
target_link_libraries(BrainIndexer
  INTERFACE
    Boost::boost
    Boost::serialization  
    Threads::Threads
    nlohmann_json::nlohmann_json
    Zisa::sfc
)
target_compile_definitions(BrainIndexer INTERFACE "-DBOOST_GEOMETRY_INDEX_DETAIL_EXPERIMENTAL")

if(SI_MPI)
  target_link_libraries(BrainIndexer INTERFACE MPI::MPI_CXX)
  target_compile_definitions(BrainIndexer INTERFACE "-DSI_MPI=1")
endif()


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
