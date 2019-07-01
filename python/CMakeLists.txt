
set(PYBIND11_SOURCES
    bind_rtree_soma.cpp
    bindings.cpp
)

pybind11_add_module(_spatial_index
                    ${PYBIND11_SOURCES}
                    ${Boost_INCLUDE_DIRS})

target_link_libraries(_spatial_index PUBLIC SpatialIndex)
