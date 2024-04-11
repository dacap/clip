// Clip Library
// Copyright (c) 2020-2022 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef CLIP_WIN_WIC_H_INCLUDED
#define CLIP_WIN_WIC_H_INCLUDED
#pragma once

#include "clip.h"

#include <algorithm>
#include <vector>

#include <shlwapi.h>
#include <wincodec.h>

namespace clip {

namespace win {

#if CLIP_ENABLE_IMAGE

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
  BitmapInfo(BITMAPV5HEADER* b5);
  BitmapInfo(BITMAPINFO* bi);

  bool is_valid() const;
  void fill_spec(image_spec& spec) const;

private:
  bool load_from(BITMAPV5HEADER* b5);
  bool load_from(BITMAPINFO* bi);
};

#endif // CLIP_ENABLE_IMAGE

// Successful calls to CoInitialize() (S_OK or S_FALSE) must match
// the calls to CoUninitialize().
// From: https://docs.microsoft.com/en-us/windows/win32/api/combaseapi/nf-combaseapi-couninitialize#remarks
struct coinit {
  HRESULT hr;
  coinit() {
    hr = CoInitialize(nullptr);
  }
  ~coinit() {
    if (hr == S_OK || hr == S_FALSE)
      CoUninitialize();
  }
};

template<class T>
class comptr {
public:
  comptr() { }
  explicit comptr(T* ptr) : m_ptr(ptr) { }
  comptr(const comptr&) = delete;
  comptr& operator=(const comptr&) = delete;
  ~comptr() { reset(); }

  T** operator&() { return &m_ptr; }
  T* operator->() { return m_ptr; }
  bool operator!() const { return !m_ptr; }

  T* get() { return m_ptr; }
  void reset() {
    if (m_ptr) {
      m_ptr->Release();
      m_ptr = nullptr;
    }
  }
private:
  T* m_ptr = nullptr;
};

#ifdef CLIP_SUPPORT_WINXP
class hmodule {
public:
  hmodule(LPCWSTR name) : m_ptr(LoadLibraryW(name)) { }
  hmodule(const hmodule&) = delete;
  hmodule& operator=(const hmodule&) = delete;
  ~hmodule() {
    if (m_ptr)
      FreeLibrary(m_ptr);
  }

  operator HMODULE() { return m_ptr; }
  bool operator!() const { return !m_ptr; }
private:
  HMODULE m_ptr = nullptr;
};
#endif

//////////////////////////////////////////////////////////////////////
// Encode the image as PNG format

bool write_png_on_stream(const image& image,
                         IStream* stream);

HGLOBAL write_png(const image& image);

//////////////////////////////////////////////////////////////////////
// Decode the clipboard data from PNG format

bool read_png(const uint8_t* buf,
              const UINT len,
              image* output_image,
              image_spec* output_spec);


// Fills the output_img with the data provided by the BitmapInfo structure.
// Returns true if it was able to fill the output image or false otherwise.
bool create_img(const win::BitmapInfo& bi, image& output_img);

// Returns a handle to the HGLOBAL memory reserved to create a DIBV5 based
// on the image passed by parameter. Returns null if it cannot create the handle.
HGLOBAL create_dibv5(const image& image);

} // namespace win
} // namespace clip

#endif
