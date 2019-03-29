
set(SI_HEADERS
	${si_include_directory}/spatial_index/bounding_box.hpp
        ${si_include_directory}/spatial_index/entries.hpp
	${si_include_directory}/spatial_index/rtree_points.hpp
	${si_include_directory}/spatial_index/rtree_spheres.hpp

)

set(PYBIND11_SOURCES
	bindings.cpp
)

set(PYBIND11_CPP_STANDARD -std=c++11)

pybind11_add_module(_spatial_index
					${PYBIND11_SOURCES}
					${SI_HEADERS}
					${Boost_INCLUDE_DIRS})

target_link_libraries(_spatial_index PUBLIC SpatialIndex)

target_include_directories(_spatial_index INTERFACE ${si_include_directory})
