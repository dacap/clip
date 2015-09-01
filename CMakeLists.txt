# Clip Library
# Copyright (C) 2015 David Capello

cmake_minimum_required(VERSION 3.2)

project(clip CXX)
option(CLIP_EXAMPLES "Compile clip examples" on)

include_directories(.)
set(CLIP_SOURCES clip.cpp)

if(WIN32)
  add_definitions(-D_SCL_SECURE_NO_WARNINGS)
endif()

if(APPLE)
  list(APPEND CLIP_SOURCES clip_osx.mm)
endif()

add_library(clip ${CLIP_SOURCES})

if(APPLE)
  find_library(COCOA_LIBRARY Cocoa)
  target_link_libraries(clip ${COCOA_LIBRARY})
endif()

if(CLIP_EXAMPLES)
  add_subdirectory(examples)
endif()
