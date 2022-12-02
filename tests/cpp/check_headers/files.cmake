target_sources(si_check_headers
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/util.cpp
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/sort_tile_recursion.cpp
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/geometries.cpp
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/multi_index.cpp
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/distributed_sort_tile_recursion.cpp
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/point3d.cpp
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/logging.cpp
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/version.cpp
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/meta_data.cpp
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/mpi_wrapper.cpp
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/index.cpp
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/distributed_sorting.cpp
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/distributed_analysis.cpp
)
