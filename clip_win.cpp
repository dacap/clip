// Clip Library
// Copyright (C) 2015-2020  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "clip_win.h"

#include "clip.h"
#include "clip_lock_impl.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace clip {

namespace {

// Data type used as header for custom formats to indicate the exact
// size of the user custom data. This is necessary because it looks
// like GlobalSize() might not return the exact size, but a greater
// value.
typedef uint64_t CustomSizeT;

class Hglobal {
public:
  Hglobal() : m_handle(nullptr) {
  }

  explicit Hglobal(HGLOBAL handle) : m_handle(handle) {
  }

  explicit Hglobal(size_t len) : m_handle(GlobalAlloc(GHND, len)) {
  }

  ~Hglobal() {
    if (m_handle)
      GlobalFree(m_handle);
  }

  void release() {
    m_handle = nullptr;
  }

  operator HGLOBAL() {
    return m_handle;
  }

private:
  HGLOBAL m_handle;
};


// From: https://issues.chromium.org/issues/40080988#comment8
//
//  "Adds impersonation of the anonymous token around calls to the
//   CloseClipboard() system call. On Windows 8+ the win32k driver
//   captures the access token of the caller and makes it available to
//   other users on the desktop through the system call
//   GetClipboardAccessToken(). This introduces a risk of privilege
//   escalation in sandboxed processes. By performing the
//   impersonation then whenever Chrome writes data to the clipboard
//   only the anonymous token is available."
//
class AnonymousTokenImpersonator {
public:
  AnonymousTokenImpersonator()
    : m_must_revert(ImpersonateAnonymousToken(GetCurrentThread()))
  {}

  ~AnonymousTokenImpersonator() {
    if (m_must_revert)
      RevertToSelf();
  }
private:
  const bool m_must_revert;
};

}

lock::impl::impl(void* hwnd) : m_locked(false) {
  for (int i=0; i<5; ++i) {
    if (OpenClipboard((HWND)hwnd)) {
      m_locked = true;
      break;
    }
    Sleep(20);
  }

  if (!m_locked) {
    error_handler e = get_error_handler();
    if (e)
      e(ErrorCode::CannotLock);
  }
}

lock::impl::~impl() {
  if (m_locked) {
    AnonymousTokenImpersonator guard;
    CloseClipboard();
  }
}

bool lock::impl::clear() {
  return (EmptyClipboard() ? true: false);
}

bool lock::impl::is_convertible(format f) const {
  if (f == text_format()) {
    return
      (IsClipboardFormatAvailable(CF_TEXT) ||
       IsClipboardFormatAvailable(CF_UNICODETEXT) ||
       IsClipboardFormatAvailable(CF_OEMTEXT));
  }
#if CLIP_ENABLE_IMAGE
  else if (f == image_format()) {
    return (IsClipboardFormatAvailable(CF_DIB) ? true: false);
  }
#endif // CLIP_ENABLE_IMAGE
  else if (IsClipboardFormatAvailable(f))
    return true;
  else
    return false;
}

bool lock::impl::set_data(format f, const char* buf, size_t len) {
  bool result = false;

  if (f == text_format()) {
    if (len > 0) {
      int reqsize = MultiByteToWideChar(CP_UTF8, 0, buf, len, NULL, 0);
      if (reqsize > 0) {
        ++reqsize;

        Hglobal hglobal(sizeof(WCHAR)*reqsize);
        LPWSTR lpstr = static_cast<LPWSTR>(GlobalLock(hglobal));
        MultiByteToWideChar(CP_UTF8, 0, buf, len, lpstr, reqsize);
        GlobalUnlock(hglobal);

        result = (SetClipboardData(CF_UNICODETEXT, hglobal)) ? true: false;
        if (result)
          hglobal.release();
      }
    }
  }
  else {
    Hglobal hglobal(len+sizeof(CustomSizeT));
    if (hglobal) {
      auto dst = (uint8_t*)GlobalLock(hglobal);
      if (dst) {
        *((CustomSizeT*)dst) = len;
        memcpy(dst+sizeof(CustomSizeT), buf, len);
        GlobalUnlock(hglobal);
        result = (SetClipboardData(f, hglobal) ? true: false);
        if (result)
          hglobal.release();
      }
    }
  }

  return result;
}

bool lock::impl::get_data(format f, char* buf, size_t len) const {
  assert(buf);

  if (!buf || !is_convertible(f))
    return false;

  bool result = false;

  if (f == text_format()) {
    if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
      HGLOBAL hglobal = GetClipboardData(CF_UNICODETEXT);
      if (hglobal) {
        LPWSTR lpstr = static_cast<LPWSTR>(GlobalLock(hglobal));
        if (lpstr) {
          size_t reqsize =
            WideCharToMultiByte(CP_UTF8, 0, lpstr, -1,
                                nullptr, 0, nullptr, nullptr);

          assert(reqsize <= len);
          if (reqsize <= len) {
            WideCharToMultiByte(CP_UTF8, 0, lpstr, -1,
                                buf, reqsize, nullptr, nullptr);
            result = true;
          }
          GlobalUnlock(hglobal);
        }
      }
    }
    else if (IsClipboardFormatAvailable(CF_TEXT)) {
      HGLOBAL hglobal = GetClipboardData(CF_TEXT);
      if (hglobal) {
        LPSTR lpstr = static_cast<LPSTR>(GlobalLock(hglobal));
        if (lpstr) {
          // TODO check length
          memcpy(buf, lpstr, len);
          result = true;
          GlobalUnlock(hglobal);
        }
      }
    }
  }
  else {
    if (IsClipboardFormatAvailable(f)) {
      HGLOBAL hglobal = GetClipboardData(f);
      if (hglobal) {
        const SIZE_T total_size = GlobalSize(hglobal);
        auto ptr = (const uint8_t*)GlobalLock(hglobal);
        if (ptr) {
          CustomSizeT reqsize = *((CustomSizeT*)ptr);

          // If the registered length of data in the first CustomSizeT
          // number of bytes of the hglobal data is greater than the
          // GlobalSize(hglobal), something is wrong, it should not
          // happen.
          assert(reqsize <= total_size);
          if (reqsize > total_size)
            reqsize = total_size - sizeof(CustomSizeT);

          if (reqsize <= len) {
            memcpy(buf, ptr+sizeof(CustomSizeT), reqsize);
            result = true;
          }
          GlobalUnlock(hglobal);
        }
      }
    }
  }

  return result;
}

size_t lock::impl::get_data_length(format f) const {
  size_t len = 0;

  if (f == text_format()) {
    if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
      HGLOBAL hglobal = GetClipboardData(CF_UNICODETEXT);
      if (hglobal) {
        LPWSTR lpstr = static_cast<LPWSTR>(GlobalLock(hglobal));
        if (lpstr) {
          len =
            WideCharToMultiByte(CP_UTF8, 0, lpstr, -1,
                                nullptr, 0, nullptr, nullptr);
          GlobalUnlock(hglobal);
        }
      }
    }
    else if (IsClipboardFormatAvailable(CF_TEXT)) {
      HGLOBAL hglobal = GetClipboardData(CF_TEXT);
      if (hglobal) {
        LPSTR lpstr = (LPSTR)GlobalLock(hglobal);
        if (lpstr) {
          len = strlen(lpstr) + 1;
          GlobalUnlock(hglobal);
        }
      }
    }
  }
  else if (f != empty_format()) {
    if (IsClipboardFormatAvailable(f)) {
      HGLOBAL hglobal = GetClipboardData(f);
      if (hglobal) {
        const SIZE_T total_size = GlobalSize(hglobal);
        auto ptr = (const uint8_t*)GlobalLock(hglobal);
        if (ptr) {
          len = *((CustomSizeT*)ptr);

          assert(len <= total_size);
          if (len > total_size)
            len = total_size - sizeof(CustomSizeT);

          GlobalUnlock(hglobal);
        }
      }
    }
  }

  return len;
}

#if CLIP_ENABLE_IMAGE

bool lock::impl::set_image(const image& image) {
  const image_spec& spec = image.spec();

  // Add the PNG clipboard format for images with alpha channel
  // (useful to communicate with some Windows programs that only use
  // alpha data from PNG clipboard format)
  if (spec.bits_per_pixel == 32 &&
      spec.alpha_mask) {
    UINT png_format = RegisterClipboardFormatA("PNG");
    if (png_format) {
      Hglobal png_handle(win::write_png(image));
      if (png_handle)
        SetClipboardData(png_format, png_handle);
    }
  }

  Hglobal hmem(clip::win::create_dibv5(image));
  if (!hmem)
    return false;

  SetClipboardData(CF_DIBV5, hmem);
  return true;
}

bool lock::impl::get_image(image& output_img) const {
  // Get the "PNG" clipboard format (this is useful only for 32bpp
  // images with alpha channel, in other case we can use the regular
  // DIB format)
  UINT png_format = RegisterClipboardFormatA("PNG");
  if (png_format && IsClipboardFormatAvailable(png_format)) {
    HANDLE png_handle = GetClipboardData(png_format);
    if (png_handle) {
      size_t png_size = GlobalSize(png_handle);
      uint8_t* png_data = (uint8_t*)GlobalLock(png_handle);
      bool result = win::read_png(png_data, png_size, &output_img, nullptr);
      GlobalUnlock(png_handle);
      if (result)
        return true;
    }
  }

  win::BitmapInfo bi;
  return bi.to_image(output_img);
}

bool lock::impl::get_image_spec(image_spec& spec) const {
  UINT png_format = RegisterClipboardFormatA("PNG");
  if (png_format && IsClipboardFormatAvailable(png_format)) {
    HANDLE png_handle = GetClipboardData(png_format);
    if (png_handle) {
      size_t png_size = GlobalSize(png_handle);
      uint8_t* png_data = (uint8_t*)GlobalLock(png_handle);
      bool result = win::read_png(png_data, png_size, nullptr, &spec);
      GlobalUnlock(png_handle);
      if (result)
        return true;
    }
  }

  win::BitmapInfo bi;
  if (!bi.is_valid())
    return false;
  bi.fill_spec(spec);
  return true;
}

#endif // CLIP_ENABLE_IMAGE

format register_format(const std::string& name) {
  int reqsize = 1+MultiByteToWideChar(CP_UTF8, 0,
                                      name.c_str(), name.size(), NULL, 0);
  std::vector<WCHAR> buf(reqsize);
  MultiByteToWideChar(CP_UTF8, 0, name.c_str(), name.size(),
                      &buf[0], reqsize);

  // From MSDN, registered clipboard formats are identified by values
  // in the range 0xC000 through 0xFFFF.
  return (format)RegisterClipboardFormatW(&buf[0]);
}

} // namespace clip
