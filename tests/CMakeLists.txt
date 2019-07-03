
add_definitions("-Wfatal-errors")


set(test_src "unit_tests")
add_executable(${test_src} cpp/${test_src}.cpp)
target_link_libraries(${test_src} SpatialIndex Boost::unit_test_framework)
add_test(NAME ${test_src} COMMAND ${test_src})
if(NOT Boost_USE_STATIC_LIBS)
    target_compile_definitions(${test_src} PUBLIC "-DBOOST_TEST_DYN_LINK=TRUE")
endif()


# Base example uses serialization
set(BUILD_BASENAME "test_clean")
add_executable(${BUILD_BASENAME} cpp/examples/${BUILD_BASENAME}.cpp)
target_link_libraries(${BUILD_BASENAME} SpatialIndex)
add_test(NAME ${BUILD_BASENAME} COMMAND ${BUILD_BASENAME})


# Python tests
if(SI_PYTHON_BINDINGS)
    add_test(NAME PyBindings
             COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/test_rtree_soma.py)
    set_tests_properties(PyBindings PROPERTIES ENVIRONMENT
                         PYTHONPATH=${CMAKE_BINARY_DIR}/python:$ENV{PYTHONPATH})
endif()



