
find_package(Python3 COMPONENTS Interpreter REQUIRED)

function(minify name impflag guard headers sources)
  if (NOT ${impflag} STREQUAL "")
    set(impflag "-d" "${impflag}")
  endif()
  if (NOT ${guard} STREQUAL "")
    set(guard "-g" "${guard}")
  endif()
  add_custom_target(${name} ${Python3_EXECUTABLE}
    "${CMAKE_CURRENT_LIST_DIR}/buildconf/mini.py"
    "-o" "${CMAKE_BINARY_DIR}/${name}"
    ${impflag}
    ${guard}
    "-h" "\"${headers}\""
    "\"${sources}\""
    WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}")
endfunction()

function(minify2)
  set(options)
  set(oneValueArgs HEADER SOURCE IMPLEMENT_FLAG)
  set(multiValueArgs HEADERS SOURCES)
  cmake_parse_arguments(PARSE_ARGV 0 arg
    "${options}" "${oneValueArgs}" "${multiValueArgs}"
  )
  get_filename_component(name ${arg_HEADER} NAME)
  string(REPLACE "." "_" fixed_name ${name})
  if (NOT DEFINED arg_SOURCE)
    set(arg_SOURCE "")
  else()
    set(arg_SOURCE "-O" "${arg_SOURCE}")
  endif()
  if (NOT DEFINED arg_IMPLEMENT_FLAG)
    set(arg_IMPLEMENT_FLAG "")
  else()
    set(arg_IMPLEMENT_FLAG "-d" "${arg_IMPLEMENT_FLAG}")
  endif()

  add_custom_target(${name} ${Python3_EXECUTABLE}
    "${CMAKE_CURRENT_LIST_DIR}/buildconf/mini.py"
    "-o" "${arg_HEADER}"
    ${arg_SOURCE}
    ${arg_IMPLEMENT_FLAG}
    "-g" "${fixed_name}"
    "-h" "\"${arg_HEADERS}\""
    "\"${arg_SOURCES}\""
    WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}")
endfunction()