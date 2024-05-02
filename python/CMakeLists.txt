
set(PY_BINDINGS_SOURCES py_bindings.cpp)

pybind11_add_module(_brain_indexer  ${PY_BINDINGS_SOURCES})

target_link_libraries(_brain_indexer PUBLIC BrainIndexer)
target_compile_definitions(_brain_indexer PUBLIC "-DSI_FOR_PYBIND=1")

if(NOT EXTENSION_OUTPUT_DIRECTORY)
    set(EXTENSION_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/brain_indexer)
endif()
set_target_properties(_brain_indexer PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${EXTENSION_OUTPUT_DIRECTORY})

install(TARGETS _brain_indexer LIBRARY DESTINATION ${SKBUILD_PLATLIB_DIR}/brain_indexer)
