# Copy-Paste Library
# Copyright (C) 2015 David Capello

cmake_minimum_required(VERSION 3.2)

project(copypaste CXX)
option(COPYPASTE_EXAMPLES "Compile copypaste examples" on)

include_directories(.)
if(WIN32)
  add_definitions(-D_SCL_SECURE_NO_WARNINGS)
endif()

add_library(copypaste copypaste.cpp)

if(COPYPASTE_EXAMPLES)
  add_subdirectory(examples)
endif()
