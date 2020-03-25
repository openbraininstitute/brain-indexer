
set(PYBIND11_SOURCES rtree_index.cpp
                     spatial_index.cpp)

pybind11_add_module(_spatial_index  ${PYBIND11_SOURCES})

target_link_libraries(_spatial_index PUBLIC SpatialIndex)

if(NOT EXTENSION_OUTPUT_DIRECTORY)
    set(EXTENSION_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/spatial_index)
endif()
set_target_properties(_spatial_index PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${EXTENSION_OUTPUT_DIRECTORY})
