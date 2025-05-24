cmake_minimum_required(VERSION "3.22")

project("SCL" VERSION "1.0" DESCRIPTION "Stormground C/C++ Library" LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 14)

include("scripts/clangdConfig.cmake")
include("scripts/mini.cmake")

include_directories("src" "${CMAKE_BINARY_DIR}")

set(SCL_HEADERS
  "src/sclcore.hpp"
  "src/scldict.hpp"
  "src/sclpath.hpp"
  "src/sclxml.hpp"
  "src/scljobs.hpp")

  
set(SCL_SOURCES
  "src/sclcore.cpp"
  "src/sclpath.cpp"
  "src/scljobs.cpp")
  
set(SCL_PACK_HEADERS
  "src/sclpack.hpp")

set(SCL_PACK_SOURCES
  "src/sclpack.cpp"
  "src/lz4/lz4.c")


minify(miniscl SCL_IMPL "" "${SCL_HEADERS}" "${SCL_SOURCES}")

add_executable(test "examples/test.cpp" ${SCL_SOURCES})
#add_dependencies(test miniscl)
set_target_properties(test PROPERTIES CMAKE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/src")
clangdConfigure(test)

add_executable(package "examples/package.cpp" ${SCL_SOURCES})
