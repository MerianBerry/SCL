cmake_minimum_required(VERSION "3.22")

project("SCL" VERSION "1.0" DESCRIPTION "Stormground C/C++ Library" LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_C_STANDARD 11)

if (PROJECT_IS_TOP_LEVEL)
  set(SCL_EXTRAS_DEFAULT ON)
  else()
  set(SCL_EXTRAS_DEFAULT OFF)
endif()


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

set(SCLC_HEADERS
  "include/sclc_path.h"
  "include/sclc_string.h"
  "include/sclc_time.h"
 )

set(SCLC_SOURCES
  "src/sclc_path.c"
  "src/sclc_string.c"
  "src/sclc_swiss.c"
  "src/sclc_time.c"
  "src/xxhash.c"
 )

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(scl ${SCL_SOURCES})
add_library(sclc ${SCLC_SOURCES})
#target_compile_options(sclc PUBLIC "/arch:AVX2")

get_target_property(sclc_type sclc TYPE)
if (${sclc_type} STREQUAL "SHARED_LIBRARY")
  target_compile_definitions(sclc PRIVATE "SCL_DLL=1")
endif()

minify(miniscl.hpp SCL_IMPL "" "${SCL_HEADERS}" "${SCL_SOURCES}")
minify2(HEADER "sclc.h" SOURCE "sclc.c"
  IMPLEMENT_FLAG SCLC_IMPL
  HEADERS ${SCLC_HEADERS}
  SOURCES ${SCLC_SOURCES})

if (NOT DEFINED SCL_TESTS)
  set(SCL_TESTS ${SCL_EXTRAS_DEFAULT})
endif()

if (SCL_TESTS)
  add_subdirectory(test)
endif()

if (NOT DEFINED SCL_EXAMPLES)
  set(SCL_EXAMPLES ${SCL_EXTRAS_DEFAULT})
endif()

if (SCL_EXAMPLES)
  add_executable(package "examples/package.cpp" ${SCL_SOURCES})
endif()
#target_compile_definitions(package PUBLIC LZ4F_STATIC_LINKING_ONLY)
clangd(
  INLAY_HINTS
  INLAY_PARAMETER_NAMES
)