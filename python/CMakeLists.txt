
set(PY_BINDINGS_SOURCES py_bindings.cpp)

pybind11_add_module(_spatial_index  ${PY_BINDINGS_SOURCES})

target_link_libraries(_spatial_index PUBLIC SpatialIndex)
target_compile_definitions(_spatial_index PUBLIC "-DSI_FOR_PYBIND=1")

if(NOT EXTENSION_OUTPUT_DIRECTORY)
    set(EXTENSION_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/spatial_index)
endif()
set_target_properties(_spatial_index PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${EXTENSION_OUTPUT_DIRECTORY})

install(TARGETS _spatial_index LIBRARY DESTINATION ${SKBUILD_PLATLIB_DIR}/spatial_index)
