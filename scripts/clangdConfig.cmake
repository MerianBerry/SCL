
function(clangdConfigure target)
get_target_property(inc_dirs ${target} INCLUDE_DIRECTORIES)
get_target_property(cxx_standard ${target} CXX_STANDARD)
if (${cxx_standard} STREQUAL "cxx_standard-NOTFOUND")
set(cxx_standard "14")
endif()
#message(STATUS "DIRS: ${inc_dirs}")
set(CLANGD_INCLUDE_DIRS "")

foreach(dir ${inc_dirs})
set(CLANGD_INCLUDE_DIRS "${CLANGD_INCLUDE_DIRS}, -I${dir}")
endforeach()

file(WRITE "${CMAKE_CURRENT_LIST_DIR}/.clangd" "If:
  PathMatch: [.*\.hpp, .*\.h, .*\.cpp, .*\.c]
  PathExclude: third_party/*
CompileFlags:
  Add: [-std=c++${cxx_standard}, -Wno-deprecated-declarations, -Wno-unused-includes${CLANGD_INCLUDE_DIRS}]
")

endfunction()

