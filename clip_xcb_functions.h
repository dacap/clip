// Clip Library
// Copyright (c) 2015-2026 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.
// 
// This file was originally copied from VTK, under BSD-3-Clause license
// Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen

#ifndef CLIB_XCB_FUNCTIONS_H_INCLUDED
#define CLIB_XCB_FUNCTIONS_H_INCLUDED

#define CLIP_XCB_EXPORT __attribute__((visibility("default")))

#include <xcb/xcb.h>

extern "C"
{
  typedef xcb_connection_t* (*clip_xcb_connect_type)(const char *, int *);
  
  CLIP_XCB_EXPORT extern clip_xcb_connect_type clip_xcb_connect;

  /**
   * Initialize the xcb function pointers by dynamically loading them from libxcb.so.
   * Must be called before using any of the function pointers.
   * Safe to call multiple times; subsequent calls have no effect.
   */
  CLIP_XCB_EXPORT void clip_xcb_functions_initialize();

  /**
   * Finalize the xcb function pointers, releasing any resources.
   * Should be called when done using the function pointers.
   * Safe to call multiple times; subsequent calls have no effect.
   */
  CLIP_XCB_EXPORT void clip_xcb_functions_finalize();
}
#endif // CLIB_XCB_FUNCTIONS_H_INCLUDED
