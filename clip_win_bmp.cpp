// Clip Library
// Copyright (c) 2015-2024  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "clip_win_bmp.h"

#include "clip.h"

namespace clip {
namespace win {

namespace {

unsigned long get_shift_from_mask(unsigned long mask) {
  unsigned long shift = 0;
  for (shift=0; shift<sizeof(unsigned long)*8; ++shift)
    if (mask & (1 << shift))
      return shift;
  return shift;
}

} // anonymous namespace

BitmapInfo::BitmapInfo() {
  // Use DIBV5 only for 32 bpp uncompressed bitmaps and when all
  // masks are valid.
  if (IsClipboardFormatAvailable(CF_DIBV5)) {
    b5 = (BITMAPV5HEADER*)GetClipboardData(CF_DIBV5);
    if (b5 &&
        b5->bV5BitCount == 32 &&
        ((b5->bV5Compression == BI_RGB) ||
         (b5->bV5Compression == BI_BITFIELDS &&
          b5->bV5RedMask && b5->bV5GreenMask &&
          b5->bV5BlueMask && b5->bV5AlphaMask))) {
      width       = b5->bV5Width;
      height      = b5->bV5Height;
      bit_count   = b5->bV5BitCount;
      compression = b5->bV5Compression;
      if (compression == BI_BITFIELDS) {
        red_mask    = b5->bV5RedMask;
        green_mask  = b5->bV5GreenMask;
        blue_mask   = b5->bV5BlueMask;
        alpha_mask  = b5->bV5AlphaMask;
      }
      else {
        red_mask    = 0xff0000;
        green_mask  = 0xff00;
        blue_mask   = 0xff;
        alpha_mask  = 0xff000000;
      }
      return;
    }
  }

  if (IsClipboardFormatAvailable(CF_DIB))
    bi = (BITMAPINFO*)GetClipboardData(CF_DIB);
  if (!bi)
    return;

  width       = bi->bmiHeader.biWidth;
  height      = bi->bmiHeader.biHeight;
  bit_count   = bi->bmiHeader.biBitCount;
  compression = bi->bmiHeader.biCompression;

  if (compression == BI_BITFIELDS) {
    red_mask   = *((uint32_t*)&bi->bmiColors[0]);
    green_mask = *((uint32_t*)&bi->bmiColors[1]);
    blue_mask  = *((uint32_t*)&bi->bmiColors[2]);
    if (bit_count == 32)
      alpha_mask = 0xff000000;
  }
  else if (compression == BI_RGB) {
    switch (bit_count) {
      case 32:
        red_mask   = 0xff0000;
        green_mask = 0xff00;
        blue_mask  = 0xff;
        alpha_mask = 0xff000000;
        break;
      case 24:
      case 8: // We return 8bpp images as 24bpp
        red_mask   = 0xff0000;
        green_mask = 0xff00;
        blue_mask  = 0xff;
        break;
      case 16:
        red_mask   = 0x7c00;
        green_mask = 0x03e0;
        blue_mask  = 0x001f;
        break;
    }
  }
}

void BitmapInfo::fill_spec(image_spec& spec) {
  spec.width = width;
  spec.height = (height >= 0 ? height: -height);
  // We convert indexed to 24bpp RGB images to match the OS X behavior
  spec.bits_per_pixel = bit_count;
  if (spec.bits_per_pixel <= 8)
    spec.bits_per_pixel = 24;
  spec.bytes_per_row = width*((spec.bits_per_pixel+7)/8);
  spec.red_mask   = red_mask;
  spec.green_mask = green_mask;
  spec.blue_mask  = blue_mask;
  spec.alpha_mask = alpha_mask;

  switch (spec.bits_per_pixel) {

    case 24: {
      // We need one extra byte to avoid a crash updating the last
      // pixel on last row using:
      //
      //   *((uint32_t*)ptr) = pixel24bpp;
      //
      ++spec.bytes_per_row;

      // Align each row to 32bpp
      int padding = (4-(spec.bytes_per_row&3))&3;
      spec.bytes_per_row += padding;
      break;
    }

    case 16: {
      int padding = (4-(spec.bytes_per_row&3))&3;
      spec.bytes_per_row += padding;
      break;
    }
  }

  unsigned long* masks = &spec.red_mask;
  unsigned long* shifts = &spec.red_shift;
  for (unsigned long* shift=shifts, *mask=masks; shift<shifts+4; ++shift, ++mask) {
    if (*mask)
      *shift = get_shift_from_mask(*mask);
  }
}

} // namespace win
} // namespace clip
