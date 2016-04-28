# Clip Library
# Copyright (C) 2015-2016 David Capello

function(add_example name)
  add_executable(${name} ${name}.cpp)
  target_link_libraries(${name} clip)
endfunction()

add_example(helloworld)
add_example(image)
add_example(int_format)
add_example(paste)
