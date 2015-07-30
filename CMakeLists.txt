# Copy-Paste Library
# Copyright (C) 2015 David Capello

cmake_minimum_required(VERSION 3.2)

project(copypaste CXX)
option(COPYPASTE_EXAMPLES "Compile copypaste examples" on)

include_directories(.)
set(COYPASTE_SOURCES copypaste.cpp)

if(WIN32)
  add_definitions(-D_SCL_SECURE_NO_WARNINGS)
endif()

if(APPLE)
  list(APPEND COYPASTE_SOURCES copypaste_osx.mm)
endif()

add_library(copypaste ${COYPASTE_SOURCES})

if(APPLE)
  find_library(COCOA_LIBRARY Cocoa)
  target_link_libraries(copypaste ${COCOA_LIBRARY})
endif()

if(COPYPASTE_EXAMPLES)
  add_subdirectory(examples)
endif()
