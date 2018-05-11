# Clip Library
# Copyright (C) 2015-2018 David Capello

cmake_minimum_required(VERSION 2.6)

project(clip)
option(CLIP_EXAMPLES "Compile clip examples" on)

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
  endif()
endif()

if(CLIP_EXAMPLES)
  add_subdirectory(examples)
endif()
