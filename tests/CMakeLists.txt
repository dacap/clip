# Clip Library
# Copyright (C) 2018 David Capello

function(add_clip_test name)
  add_executable(${name} ${name}.cpp)
  add_test(NAME ${name} COMMAND ${name})
  target_link_libraries(${name} clip)
endfunction()

add_clip_test(text_tests)
