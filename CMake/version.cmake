
function(sanitize_version output version)
  # keep MAJOR.MINOR.PATCH only
  string(REGEX
         REPLACE "v?([0-9]+\\.[0-9]+(\\.[0-9]+)?).*"
                 "\\1"
                 version
                 "${version}")
  set(${output} "${version}" PARENT_SCOPE)
endfunction()


function(get_git_version output)
  execute_process(COMMAND git describe --tags
                  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                  RESULT_VARIABLE GIT_VERSION_FAILED
                  OUTPUT_VARIABLE GIT_PKG_VERSION_FULL
                  ERROR_VARIABLE GIT_VERSION_ERROR
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(GIT_VERSION_FAILED)
    message(
      FATAL_ERROR
        "Could not retrieve version from command 'git describe --tags'\n"
        ${GIT_VERSION_ERROR})
  endif()

  # keep last line of command output
  string(REPLACE "\n"
                 ";"
                 GIT_PKG_VERSION_FULL
                 "${GIT_PKG_VERSION_FULL}")
  list(GET GIT_PKG_VERSION_FULL -1 version)

  sanitize_version(version ${version})

  set(${output} "${version}" PARENT_SCOPE)

endfunction()
