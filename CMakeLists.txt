cmake_minimum_required(VERSION "3.22")

project("SCL" VERSION "1.0" DESCRIPTION "Stormground C/C++ Library" LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 14)

include("buildconf/clangd.cmake")
include("buildconf/mini.cmake")

include_directories("src" "${CMAKE_BINARY_DIR}")

if (MSVC AND OFF)
  add_compile_options("/Wall" "/wd5045" "/wd4820" "/wd4100" "/wd4514")
endif()

set(SCL_HEADERS
  "include/scl.hpp")
  
set(SCL_SOURCES
  "src/hash.cpp"
  "src/jobs.cpp"
  "src/misc.cpp"
  "src/pack.cpp"
  "src/path.cpp"
  "src/reduce.cpp"
  "src/stream.cpp"
  "src/string.cpp"
  "src/time.cpp"
  "src/lz4/lz4.c"
  "src/lz4/lz4frame.c"
  "src/lz4/lz4hc.c"
  "src/lz4/xxhash.c")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(scl ${SCL_SOURCES})

minify(miniscl.hpp SCL_IMPL "" "${SCL_HEADERS}" "${SCL_SOURCES}")

if (NOT DEFINED SCL_TESTS)
  set(SCL_TESTS TRUE)
endif()

if (SCL_TESTS)
  add_subdirectory(test)
endif()

if (NOT DEFINED SCL_EXAMPLES)
  set(SCL_EXAMPLES TRUE)
endif()

if (SCL_EXAMPLES)
  add_executable(package "examples/package.cpp" ${SCL_SOURCES})
endif()
#target_compile_definitions(package PUBLIC LZ4F_STATIC_LINKING_ONLY)
clangd(
  INLAY_HINTS
  INLAY_PARAMETER_NAMES
)