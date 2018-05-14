# Clip Library
# Copyright (C) 2015-2018 David Capello

cmake_minimum_required(VERSION 2.6)

project(clip)
option(CLIP_EXAMPLES "Compile clip examples" on)
if(UNIX AND NOT APPLE)
  option(CLIP_X11_WITH_PNG "Compile with libpng to support copy/paste image in png format" on)
endif()

include_directories(.)
set(CLIP_SOURCES clip.cpp)

if(WIN32)
  add_definitions(-D_SCL_SECURE_NO_WARNINGS)
  list(APPEND CLIP_SOURCES clip_win.cpp)
elseif(APPLE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -stdlib=libc++")
  add_definitions(-fobjc-arc)
  list(APPEND CLIP_SOURCES clip_osx.mm)
elseif(UNIX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

  include(CheckIncludeFiles)
  check_include_files(xcb/xcb.h HAVE_XCB_XLIB_H)

  if(HAVE_XCB_XLIB_H)
    add_definitions(-DHAVE_XCB_XLIB_H)

    if(CLIP_X11_WITH_PNG)
      check_include_files(png.h HAVE_PNG_H)
      if(CLIP_X11_PNG_LIBRARY)
        set(PNG_LIBRARY ${CLIP_X11_PNG_LIBRARY})
      else()
        find_library(PNG_LIBRARY png)
      endif()
      if(HAVE_PNG_H AND PNG_LIBRARY)
        add_definitions(-DHAVE_PNG_H)
      endif()
    endif()

    list(APPEND CLIP_SOURCES clip_x11.cpp)
  else()
    list(APPEND CLIP_SOURCES clip_none.cpp)
  endif()
else()
  list(APPEND CLIP_SOURCES clip_none.cpp)
endif()

add_library(clip ${CLIP_SOURCES})

if(APPLE)
  find_library(COCOA_LIBRARY Cocoa)
  target_link_libraries(clip ${COCOA_LIBRARY})
elseif(UNIX)
  if(HAVE_XCB_XLIB_H)
    target_link_libraries(clip xcb pthread)
    if(CLIP_X11_WITH_PNG AND HAVE_PNG_H AND PNG_LIBRARY)
      target_link_libraries(clip ${PNG_LIBRARY})
    endif()
  endif()
endif()

if(CLIP_EXAMPLES)
  add_subdirectory(examples)
endif()
