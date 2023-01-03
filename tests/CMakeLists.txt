add_definitions("-Wextra")

function (SI_UNIT_TEST_BUILD test_src)
    add_executable(${test_src} cpp/${test_src}.cpp)
    target_link_libraries(${test_src} SpatialIndex Boost::unit_test_framework)
    if(NOT Boost_USE_STATIC_LIBS)
        target_compile_definitions(${test_src} PUBLIC "-DBOOST_TEST_DYN_LINK=TRUE")
    endif()
endfunction()

function(SI_UNIT_TEST test_src)
    si_unit_test_build(${test_src})
    add_test(NAME ${test_src} COMMAND ${test_src})
endfunction()

function(SI_MPI_UNIT_TEST test_src)
    si_unit_test_build(${test_src})
    add_test(NAME ${test_src} COMMAND ${${PROJECT}_MPIEXEC} ${${PROJECT}_MPIEXEC_NPROC_FLAG} 4 "${CMAKE_CURRENT_BINARY_DIR}/${test_src}")
endfunction()

function(SI_CPP_EXAMPLE test_src)
    add_executable(${test_src} cpp/examples/${test_src}.cpp)
    target_link_libraries(${test_src} SpatialIndex)
    add_test(NAME ${test_src} COMMAND ${test_src})
endfunction()


si_unit_test("test_input_iters")
si_unit_test("unit_tests")
si_unit_test("test_geometry")
si_unit_test("test_query_ordering")
si_unit_test("test_util")

if(SI_MPI)
    si_mpi_unit_test("test_distributed_sorting")
    si_mpi_unit_test("test_multi_index")
    si_mpi_unit_test("test_random_trees")
    si_mpi_unit_test("test_serial_sort_tile_recursion")
    si_unit_test("test_distributed_sort_tile_recursion")
endif()


if(SI_BENCHMARKS)
    si_unit_test("benchmarks")
    target_link_libraries(benchmarks Boost::timer)
endif()


si_cpp_example("test_clean")

add_subdirectory(cpp/check_headers)
