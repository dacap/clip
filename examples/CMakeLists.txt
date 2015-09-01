# Copy-Paste Library
# Copyright (C) 2015 David Capello

function(add_example name)
  add_executable(${name} ${name}.cpp)
  target_link_libraries(${name} copypaste)
endfunction()

add_example(helloworld)
add_example(int_format)
add_example(paste)
