
set(PYBIND11_SOURCES
    spatial_index.cpp
)

pybind11_add_module(_spatial_index  ${PYBIND11_SOURCES})

target_link_libraries(_spatial_index PUBLIC SpatialIndex)
