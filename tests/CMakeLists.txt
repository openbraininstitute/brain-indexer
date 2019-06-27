
include_directories(${si_include_directory})

add_definitions("-DBOOST_GEOMETRY_INDEX_DETAIL_EXPERIMENTAL -Wfatal-errors")


set(test_src "unit_tests")
add_executable(${test_src} cpp/${test_src}.cpp)
add_test(NAME ${test_src} COMMAND ${test_src})


set(BUILD_BASENAME "test_clean")
add_executable(${BUILD_BASENAME} cpp/${BUILD_BASENAME}.cpp)
target_link_libraries(${BUILD_BASENAME} ${Boost_LIBRARIES})
