
find_package(Python3 COMPONENTS Interpreter REQUIRED)

function(minify name impflag guard headers sources)
  if (NOT ${impflag} STREQUAL "")
    set(impflag "-d" "${impflag}")
  endif()
  if (NOT ${guard} STREQUAL "")
    set(guard "-g" "${guard}")
  endif()
  message("headers: ${headers}")
  add_custom_target(${name} ${Python3_EXECUTABLE}
    "${CMAKE_CURRENT_LIST_DIR}/scripts/mini.py"
    "-o" "${CMAKE_BINARY_DIR}/${name}"
    ${impflag}
    ${guard}
    "-h" "\"${headers}\""
    "\"${sources}\""
    WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}")
endfunction()
