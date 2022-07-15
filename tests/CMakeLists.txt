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
    add_test(NAME ${test_src} COMMAND mpirun -np 4 "${CMAKE_CURRENT_BINARY_DIR}/${test_src}")
endfunction()

function(SI_CPP_EXAMPLE test_src)
    add_executable(${test_src} cpp/examples/${test_src}.cpp)
    target_link_libraries(${test_src} SpatialIndex)
    add_test(NAME ${test_src} COMMAND ${test_src})
endfunction()


si_unit_test("test_input_iters")
si_unit_test("unit_tests")
si_unit_test("test_grid")
si_unit_test("test_geometry")

if(SI_MPI)
    si_mpi_unit_test("test_distributed_sorting")
    si_mpi_unit_test("test_multi_index")
    si_mpi_unit_test("test_random_trees")
    si_mpi_unit_test("test_serial_sort_tile_recursion")
endif()


if(SI_BENCHMARKS)
    si_unit_test("benchmarks")
    target_link_libraries(benchmarks Boost::timer)
endif()


si_cpp_example("test_clean")
si_cpp_example("disk_mem_tree")


# Python tests
if(NOT SI_PYTHON_BINDINGS)
    return()
endif()

function(SI_PYTHON_TEST test_file)
    # We dont want to deal with python setup.py here, so our tests have a __main__
    add_test(NAME "Python_${test_file}"
             COMMAND ${PYTHON_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/${test_file}")
    set_tests_properties("Python_${test_file}" PROPERTIES ENVIRONMENT
                         PYTHONPATH=${PROJECT_SOURCE_DIR}:$ENV{PYTHONPATH})
endfunction()


si_python_test("test_rtree_sphere.py")
si_python_test("test_rtree_morph.py")
si_python_test("test_rtree_point.py")
si_python_test("test_grid.py")
si_python_test("test_index_mvd.py")
si_python_test("test_validation_FLAT.py")
