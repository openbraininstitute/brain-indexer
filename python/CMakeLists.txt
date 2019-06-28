

set(PYBIND11_SOURCES
	bind_rtree_soma.cpp
	bindings.cpp
)

include_directories(${si_include_directory})


pybind11_add_module(_spatial_index
					${PYBIND11_SOURCES}
					${Boost_INCLUDE_DIRS})

target_link_libraries(_spatial_index PUBLIC SpatialIndex)

target_include_directories(_spatial_index INTERFACE ${si_include_directory})

target_compile_definitions(_spatial_index PRIVATE
						   "-DBOOST_GEOMETRY_INDEX_DETAIL_EXPERIMENTAL")