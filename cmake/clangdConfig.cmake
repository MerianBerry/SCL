
# Configure for active languages, prioritizing CXX
get_property(languages GLOBAL PROPERTY ENABLED_LANGUAGES)
if("CXX" IN_LIST languages)
  message(STATUS "Using CXX for .clangd")
  set(CLANGD_LANG "CXX" CACHE STRING "" FORCE)
  set(CLANGD_STANDARD_FLAG "-std=c++" CACHE STRING "" FORCE)
  set(CLANGD_PATH_FILTER ".*\.h, .*\.hpp, .*\.cpp" CACHE STRING "" FORCE)
elseif("C" IN_LIST languages)
  message(STATUS "Using C for .clangd")
  set(CLANGD_LANG "C" CACHE STRING "" FORCE)
  set(CLANGD_STANDARD_FLAG "-std=c" CACHE STRING "" FORCE)
  set(CLANGD_PATH_FILTER ".*\.h, .*\.c" CACHE STRING "" FORCE)
else()
  message(WARNING "CXX and C are not enabled in this project, clangdConfigure not available")
endif()

set(CLANGD_LANG_STANDARD ${CMAKE_${CLANGD_LANG}_STANDARD})
if (NOT DEFINED CLANGD_LANG_STANDARD)
  message(SEND_ERROR "${CLANGD_LANG} standard is not defined!")
endif()
message(STATUS "${CLANGD_LANG} standard: ${CLANGD_LANG_STANDARD}")

function(clangdConfigure target)
  get_target_property(inc_dirs ${target} INCLUDE_DIRECTORIES)
  #get_target_property(${CLANGD_LANG}_standard ${target} ${CLANGD_LANG}_STANDARD)
  set(CLANGD_INCLUDE_DIRS "")

  #if(NOT DEFINED ${CLANGD_LANG}_standard)
  #  message(SEND_ERROR "Failed to get ${CLANGD_LANG} standard version")
  #endif()


  foreach(dir ${inc_dirs})
  set(CLANGD_INCLUDE_DIRS "${CLANGD_INCLUDE_DIRS}, -I${dir}")
  endforeach()

  file(WRITE "${CMAKE_CURRENT_LIST_DIR}/.clangd" 
"If:
  PathMatch: [${CLANGD_PATH_FILTER}]
  PathExclude: third_party/*
CompileFlags:
  Add: [${CLANGD_STANDARD_FLAG}${CLANGD_LANG_STANDARD}, -Wno-deprecated-declarations, -Wno-unused-includes${CLANGD_INCLUDE_DIRS}]
  ")

endfunction()

