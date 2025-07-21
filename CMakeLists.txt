cmake_minimum_required(VERSION "3.22")

project("SCL" VERSION "1.0" DESCRIPTION "Stormground C/C++ Library" LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 14)

include("cmake/clangdConfig.cmake")
include("cmake/mini.cmake")

include_directories("src" "${CMAKE_BINARY_DIR}")

set(SCL_HEADERS
  "src/sclcore.hpp"
  "src/scldict.hpp"
  "src/sclpath.hpp"
  "src/sclxml.hpp"
  "src/scljobs.hpp"
  "src/sclpack.hpp"
  "src/sclreduce.hpp")

  
set(SCL_SOURCES
  "src/sclcore.cpp"
  "src/sclpath.cpp"
  "src/scljobs.cpp"
  "src/sclpack.cpp"
  "src/sclreduce.cpp"
  "src/lz4/lz4.c"
  "src/lz4/lz4frame.c"
  "src/lz4/lz4hc.c"
  "src/lz4/xxhash.c")


minify(miniscl.hpp SCL_IMPL "" "${SCL_HEADERS}" "${SCL_SOURCES}")

add_executable(test "examples/test.cpp" ${SCL_SOURCES})
#add_dependencies(test miniscl)
set_target_properties(test PROPERTIES CMAKE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/src")
clangdConfigure(test)

add_executable(package "examples/package.cpp" ${SCL_SOURCES})
#target_compile_definitions(package PUBLIC LZ4F_STATIC_LINKING_ONLY)
