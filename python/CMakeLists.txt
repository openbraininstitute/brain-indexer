

set(PYBIND11_SOURCES
	bind_sphere_rtree2.cpp
	bindings.cpp
)

include_directories(${si_include_directory})


set(PYBIND11_CPP_STANDARD -std=c++11)

pybind11_add_module(_spatial_index
					${PYBIND11_SOURCES}
					${Boost_INCLUDE_DIRS})

target_link_libraries(_spatial_index PUBLIC SpatialIndex)

target_include_directories(_spatial_index INTERFACE ${si_include_directory})

target_compile_definitions(_spatial_index PRIVATE
						   "-DBOOST_GEOMETRY_INDEX_DETAIL_EXPERIMENTAL")