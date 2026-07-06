// Clip Library
// Copyright (c) 2015-2026 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.
// 
// This file was originally copied from VTK, under BSD-3-Clause license
// Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen

#include "clip_xcb_functions.h"

#include <string>
#include <dlfcn.h>

#define NULLIFY_POINTER_TO_FUNCTION(name) name = nullptr
#define DEFINE_POINTER_TO_FUNCTION(name) name##_type NULLIFY_POINTER_TO_FUNCTION(name)

static void* libxcb = nullptr;
DEFINE_POINTER_TO_FUNCTION(clip_xcb_connect);
DEFINE_POINTER_TO_FUNCTION(clip_xcb_get_setup);
DEFINE_POINTER_TO_FUNCTION(clip_xcb_setup_roots_iterator);
DEFINE_POINTER_TO_FUNCTION(clip_xcb_generate_id);
DEFINE_POINTER_TO_FUNCTION(clip_xcb_create_window);
DEFINE_POINTER_TO_FUNCTION(clip_xcb_destroy_window);
DEFINE_POINTER_TO_FUNCTION(clip_xcb_flush);
DEFINE_POINTER_TO_FUNCTION(clip_xcb_disconnect);

namespace
{
const char* XCB_LIBRARY_NAMES[] = { "libxcb.so.1.1.0", "libxcb.so.1", "libxcb.so", nullptr };

typedef void (*SymbolPointer)();

SymbolPointer GetSymbolAddress(void* lib, const std::string& sym)
{
  // Hack to cast pointer-to-data to pointer-to-function.
  union
  {
    void* pvoid;
    SymbolPointer psym;
  } result;
  result.pvoid = dlsym(lib, sym.c_str());
  return result.psym;
}
}

#define LOAD_POINTER_TO_FUNCTION(lib, symbol, name)                                                \
  name = reinterpret_cast<name##_type>(::GetSymbolAddress(lib, #symbol));                           \

extern "C"
{
  void clip_xcb_functions_initialize()
  {
    for (const char** libName = XCB_LIBRARY_NAMES; *libName != nullptr; ++libName)
    {
      libxcb = dlopen(*libName, RTLD_LAZY | RTLD_LOCAL);
      if (libxcb != nullptr)
      {
        break;
      }
    }
    if (libxcb == nullptr)
    {
      return;
    }
    LOAD_POINTER_TO_FUNCTION(libxcb, xcb_connect, clip_xcb_connect);
    LOAD_POINTER_TO_FUNCTION(libxcb, xcb_get_setup, clip_xcb_get_setup);
    LOAD_POINTER_TO_FUNCTION(libxcb, xcb_setup_roots_iterator, clip_xcb_setup_roots_iterator);
    LOAD_POINTER_TO_FUNCTION(libxcb, xcb_generate_id, clip_xcb_generate_id);
    LOAD_POINTER_TO_FUNCTION(libxcb, xcb_create_window, clip_xcb_create_window);
    LOAD_POINTER_TO_FUNCTION(libxcb, xcb_destroy_window, clip_xcb_destroy_window);
    LOAD_POINTER_TO_FUNCTION(libxcb, xcb_flush, clip_xcb_flush);
    LOAD_POINTER_TO_FUNCTION(libxcb, xcb_disconnect, clip_xcb_disconnect);
  }

  void clip_xcb_functions_finalize()
  {
    NULLIFY_POINTER_TO_FUNCTION(clip_xcb_connect);
    NULLIFY_POINTER_TO_FUNCTION(clip_xcb_get_setup);
    NULLIFY_POINTER_TO_FUNCTION(clip_xcb_setup_roots_iterator);
    NULLIFY_POINTER_TO_FUNCTION(clip_xcb_generate_id);
    NULLIFY_POINTER_TO_FUNCTION(clip_xcb_create_window);
    NULLIFY_POINTER_TO_FUNCTION(clip_xcb_destroy_window);
    NULLIFY_POINTER_TO_FUNCTION(clip_xcb_flush);
    NULLIFY_POINTER_TO_FUNCTION(clip_xcb_disconnect);

    if (libxcb)
    {
      dlclose(libxcb);
      libxcb = nullptr;
    }
  }
}
