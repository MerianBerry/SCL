
function(copyfile target in out)

  add_custom_command(TARGET ${target} PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${in}
      ${out}
    COMMENT "copying pre build file")
endfunction(copyfile)