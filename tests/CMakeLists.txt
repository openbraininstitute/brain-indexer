
add_definitions("-Wextra")


function(SI_UNIT_TEST test_src)
    add_executable(${test_src} cpp/${test_src}.cpp)
    target_link_libraries(${test_src} SpatialIndex Boost::unit_test_framework)
    add_test(NAME ${test_src} COMMAND ${test_src})
    if(NOT Boost_USE_STATIC_LIBS)
        target_compile_definitions(${test_src} PUBLIC "-DBOOST_TEST_DYN_LINK=TRUE")
    endif()
endfunction()


si_unit_test("test_input_iters")
si_unit_test("unit_tests")
si_unit_test("test_grid")

if(SI_BENCHMARKS)
    si_unit_test("benchmarks")
    target_link_libraries(benchmarks Boost::timer)
endif()


# Base example uses serialization
set(BUILD_BASENAME "test_clean")
add_executable(${BUILD_BASENAME} cpp/examples/${BUILD_BASENAME}.cpp)
target_link_libraries(${BUILD_BASENAME} SpatialIndex)
add_test(NAME ${BUILD_BASENAME} COMMAND ${BUILD_BASENAME})


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
