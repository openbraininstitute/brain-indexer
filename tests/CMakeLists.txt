
include_directories(${si_include_directory})


set(BUILD_BASENAME "test_clean")

add_executable(${BUILD_BASENAME} cpp/${BUILD_BASENAME}.cpp)
target_link_libraries(${BUILD_BASENAME} ${Boost_LIBRARIES})
target_compile_definitions(${BUILD_BASENAME} PRIVATE
						   "-DBOOST_GEOMETRY_INDEX_DETAIL_EXPERIMENTAL -Wfatal-errors")

