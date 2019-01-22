# Clip Library
# Copyright (C) 2018 David Capello

function(add_clip_test name)
  add_executable(clip_${name} ${name}.cpp)
  add_test(NAME clip_${name} COMMAND clip_${name})
  target_link_libraries(clip_${name} clip)
  set_tests_properties(clip_${name} PROPERTIES RUN_SERIAL TRUE)
  if(BUILD_SHARED_LIBS)
    if(WIN32)
      set_tests_properties(clip_${name} PROPERTIES ENVIRONMENT "PATH=$<TARGET_FILE_DIR:clip>;$ENV{PATH}" )
    else()
      set_tests_properties(clip_${name} PROPERTIES ENVIRONMENT "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:clip>;$ENV{LD_LIBRARY_PATH}" )
    endif()
    target_compile_definitions(clip_${name} PRIVATE "CLIP_DLL_EXPORTS" PUBLIC "CLIP_DLL")
  endif()
endfunction()

add_clip_test(text_tests)
add_clip_test(user_format_tests)
add_clip_test(image_tests)
