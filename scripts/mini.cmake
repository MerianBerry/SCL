
find_package(PythonInterp)
find_package(Python)

function(minify name impflag guard headers sources)
  if (NOT ${impflag} STREQUAL "")
    set(impflag "-d" "${impflag}")
  endif()
  if (NOT ${guard} STREQUAL "")
    set(guard "-g" "${guard}")
  endif()
  message("headers: ${headers}")
  add_custom_target(${name} ${PYTHON_EXECUTABLE}
    "${CMAKE_SOURCE_DIR}/scripts/mini.py"
    "-o" "${CMAKE_BINARY_DIR}/${name}.hpp"
    ${impflag}
    ${guard}
    "-h" "\"${headers}\""
    "\"${sources}\""
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
endfunction()