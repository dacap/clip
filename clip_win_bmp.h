// Clip Library
// Copyright (c) 2015-2024  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef CLIP_WIN_BMP_H_INCLUDED
#define CLIP_WIN_BMP_H_INCLUDED
#pragma once

#if !CLIP_ENABLE_IMAGE
  #error This file can be include only when CLIP_ENABLE_IMAGE is defined
#endif

#include <cstdint>

#include <windows.h>

namespace clip {

struct image_spec;

namespace win {

struct BitmapInfo {
  BITMAPV5HEADER* b5 = nullptr;
  BITMAPINFO* bi = nullptr;
  int width = 0;
  int height = 0;
  uint16_t bit_count = 0;
  uint32_t compression = 0;
  uint32_t red_mask = 0;
  uint32_t green_mask = 0;
  uint32_t blue_mask = 0;
  uint32_t alpha_mask = 0;

  BitmapInfo();

  bool is_valid() const {
    return (b5 || bi);
  }

  void fill_spec(image_spec& spec);
};

} // namespace win
} // namespace clip

#endif // CLIP_WIN_BMP_H_INCLUDED
