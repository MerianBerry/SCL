cmake_minimum_required(VERSION "3.22")

project("SCL" VERSION "1.0" DESCRIPTION "Stormground C/C++ Library" LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 14)

include("scripts/clangdConfig.cmake")

include_directories("src" "${CMAKE_BINARY_DIR}")

set(SCL_SOURCES
  "src/sclcore.cpp"
  "src/sclpath.cpp"
  "src/scldict.cpp")

add_custom_target(miniscl "python"
  "${CMAKE_SOURCE_DIR}/scripts/mini.py"
  "-o" "${CMAKE_BINARY_DIR}/miniscl.hpp"
  "-d" "SCL_IMPL"
  "${SCL_SOURCES}"
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

message("${SCL_SOURCES_SPACE}")

add_executable(test "examples/test.cpp" ${SCL_SOURCES})
add_dependencies(test miniscl)
set_target_properties(test PROPERTIES CMAKE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/src")
clangdConfigure(test)
