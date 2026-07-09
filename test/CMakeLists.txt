include(FetchContent)
include(../buildconf/copyfile.cmake)
include(../buildconf/clangd.cmake)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.8.1
)
FetchContent_MakeAvailable(Catch2)

add_executable(scl_tests
  tests/string.cpp
  tests/xml.cpp
)

target_link_libraries(scl_tests PRIVATE
  Catch2::Catch2WithMain
  scl
)

copyfile(scl_tests ${CMAKE_CURRENT_SOURCE_DIR}/sample.xml ${CMAKE_CURRENT_BINARY_DIR}/sample.xml)

add_executable(sclc_tests
  tests/cswiss.cpp
)

target_link_libraries(sclc_tests PRIVATE
  Catch2::Catch2WithMain
  sclc
)

include(CTest)
include(Catch)

catch_discover_tests(scl_tests)
catch_discover_tests(sclc_tests)

clangd(
  INCLUDE_DIRECTORIES
    ${catch2_SOURCE_DIR}/src/catch2
  INLAY_HINTS
  INLAY_PARAMETER_NAMES
)