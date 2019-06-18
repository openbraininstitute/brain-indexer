
set(SI_HEADERS
    ${si_include_directory}/spatial_index/bounding_box.hpp
    ${si_include_directory}/spatial_index/entries.hpp
    ${si_include_directory}/spatial_index/rtree_points.hpp
    ${si_include_directory}/spatial_index/rtree_spheres.hpp
    ${si_include_directory}/spatial_index/entries.hpp
)

set(PYBIND11_SOURCES
	bindings.cpp
)

set(PYBIND11_CPP_STANDARD -std=c++11)

# pybind11_add_module(_spatial_index
# 					${PYBIND11_SOURCES}
# 					${SI_HEADERS}
# 					${Boost_INCLUDE_DIRS})

# target_link_libraries(_spatial_index PUBLIC SpatialIndex)

# target_include_directories(_spatial_index INTERFACE ${si_include_directory})


include_directories(${si_include_directory})
set(BUILD_BASENAME "test_clean")
add_executable(${BUILD_BASENAME} ${BUILD_BASENAME}.cpp ${SI_HEADERS})
target_link_libraries(${BUILD_BASENAME} ${Boost_LIBRARIES})
target_compile_definitions(${BUILD_BASENAME} PRIVATE "-DBOOST_GEOMETRY_INDEX_DETAIL_EXPERIMENTAL")
set_property(TARGET ${BUILD_BASENAME} PROPERTY CXX_STANDARD 11)
