if(SI_BUILTIN_JSON)
  add_library(nlohmann_json INTERFACE)

  target_include_directories(nlohmann_json INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

  # Replicates the target from `find_package`.
  add_library(nlohmann_json::nlohmann_json ALIAS nlohmann_json)
endif()
