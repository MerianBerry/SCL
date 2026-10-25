cmake_minimum_required(VERSION "3.22")

project("SCL" VERSION "1.0" DESCRIPTION "Stormground C/C++ Library" LANGUAGES C)
#set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

if (PROJECT_IS_TOP_LEVEL)
  option(SCL_EXAMPLES "Configure SCL example executables" ON)
  option(SCL_TESTS "Enable SCL test executables" ON)
endif()
option(SCL_CXX "Enable SCL's c++ bindings" ON)

option(ENABLE_CLANG_TIDY "" OFF)
add_subdirectory(deps/uv)

if (MSVC AND OFF)
  add_compile_options("/Wall" "/wd5045" "/wd4820" "/wd4100" "/wd4514")
endif()

set(SCL_HEADERS
  "include/scl.hpp")
  
set(SCL_SOURCES
  src/cpp/hash.cpp
  src/cpp/jobs.cpp
  src/cpp/misc.cpp
  src/cpp/pack.cpp
  src/cpp/path.cpp
  src/cpp/reduce.cpp
  src/cpp/stream.cpp
  src/cpp/string.cpp
  src/cpp/time.cpp
  src/lz4/lz4.c
  src/lz4/lz4frame.c
  src/lz4/lz4hc.c
  src/lz4/xxhash.c)

set(SCLC_HEADERS
  "include/sclc_path.h"
  "include/sclc_string.h"
  "include/sclc_time.h"
 )

set(CORE_SOURCES
  src/fs.c
  src/glob.c
  src/pack.c
  src/path.c
  src/string.c
  src/swiss.c
  src/time.c
  src/xxhash.c
 )

include_directories(
  src
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/deps/uv/include
)

#[[
add_library(libscl SHARED ${CORE_SOURCES})
target_compile_definitions(libscl PRIVATE "SCL_DLL=1")
# Force libuv to export its functions
target_compile_definitions(uv_a
  INTERFACE
    USING_UV_SHARED=1
  PRIVATE
    BUILDING_UV_SHARED=1)
# Link the entirety of the libuv exports
target_link_libraries(libscl PRIVATE 
  $<LINK_LIBRARY:WHOLE_ARCHIVE,uv_a>)
set_target_properties(libscl PROPERTIES OUTPUT_NAME "scl")]]

add_library(libscl_a STATIC ${CORE_SOURCES})
target_link_libraries(libscl_a PUBLIC uv_a)
set_target_properties(libscl_a PROPERTIES OUTPUT_NAME "scl")
if (WIN32)
  set_target_properties(libscl_a PROPERTIES PREFIX "lib")
endif()

if (SCL_CXX OR SCL_EXAMPLES)
  #add_library(scl ${SCL_SOURCES})
  #target_link_libraries(scl PUBLIC libscl_a)
endif()

if (SCL_TESTS)
  #add_subdirectory(test)
endif()

if (SCL_EXAMPLES)
  add_executable(x "examples/test.c")
  target_link_libraries(x PRIVATE libscl_a)

  #add_executable(package "examples/package.cpp")
  #target_link_libraries(package scl)
  #target_compile_definitions(package PUBLIC LZ4F_STATIC_LINKING_ONLY)
endif()

if (PROJECT_IS_TOP_LEVEL)
  include("buildconf/clangd.cmake")
  clangd(
    INLAY_HINTS
  )
endif()


#minify(miniscl.hpp SCL_IMPL "" "${SCL_HEADERS}" "${SCL_SOURCES}")
#[[minify2(HEADER "sclc.h" SOURCE "sclc.c"
  IMPLEMENT_FLAG SCLC_IMPL
  HEADERS ${SCLC_HEADERS}
  SOURCES ${CORE_SOURCES})]]