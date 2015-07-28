// Copy-Paste Library
// Copyright (c) 2015 David Capello

#include <cassert>
#include <vector>
#include <windows.h>

namespace copypaste {

class lock::impl {
public:

  impl(void* hwnd) {
    bool opened = false;

    for (int i=0; i<5; ++i) {
      if (OpenClipboard((HWND)hwnd)) {
        opened = true;
        break;
      }
      Sleep(100);
    }

    if (!opened)
      throw std::runtime_error("Cannot open clipboard");
  }

  ~impl() {
    CloseClipboard();
  }

  bool clear() {
    return (EmptyClipboard() ? true: false);
  }

  bool is_convertible(format f) const {
    if (f == text_format()) {
      return
        (IsClipboardFormatAvailable(CF_TEXT) ||
         IsClipboardFormatAvailable(CF_UNICODETEXT) ||
         IsClipboardFormatAvailable(CF_OEMTEXT));
    }
    return false;
  }

  bool set_data(format f, const char* buf, size_t len) {
    if (f == text_format()) {
      EmptyClipboard();

      if (len > 0) {
        int reqsize = MultiByteToWideChar(CP_UTF8, 0, buf, len, NULL, 0);
        if (reqsize > 0) {
          ++reqsize;

          HGLOBAL hglobal = GlobalAlloc(GMEM_MOVEABLE |
                                        GMEM_ZEROINIT, sizeof(WCHAR)*reqsize);
          LPWSTR lpstr = static_cast<LPWSTR>(GlobalLock(hglobal));
          MultiByteToWideChar(CP_UTF8, 0, buf, len, lpstr, reqsize);
          GlobalUnlock(hglobal);

          SetClipboardData(CF_UNICODETEXT, hglobal);
        }
      }
      return true;
    }
    else
      return false;
  }

  bool get_data(format f, char* buf, size_t len) const {
    assert(buf);

    if (!buf || !is_convertible(f))
      return false;

    if (f == text_format()) {
      if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
        HGLOBAL hglobal = GetClipboardData(CF_UNICODETEXT);
        if (hglobal) {
          LPWSTR lpstr = static_cast<LPWSTR>(GlobalLock(hglobal));
          if (lpstr) {
            size_t reqsize =
              WideCharToMultiByte(CP_UTF8, 0, lpstr, wcslen(lpstr),
                                  NULL, 0, NULL, NULL) + 1;

            assert(reqsize <= len);
            if (reqsize > len) {
              // Buffer is too small
              return false;
            }

            WideCharToMultiByte(CP_UTF8, 0, lpstr, wcslen(lpstr),
                                buf, reqsize, NULL, NULL);

            GlobalUnlock(hglobal);
            return true;
          }
        }
      }
      else if (IsClipboardFormatAvailable(CF_TEXT)) {
        HGLOBAL hglobal = GetClipboardData(CF_TEXT);
        if (hglobal) {
          LPSTR lpstr = static_cast<LPSTR>(GlobalLock(hglobal));
          if (lpstr) {
            memcpy(buf, lpstr, len);
            GlobalUnlock(hglobal);
            return true;
          }
        }
      }
    }

    return false;
  }

  size_t get_data_length(format f) const {
    size_t len = 0;

    if (f == text_format()) {
      if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
        HGLOBAL hglobal = GetClipboardData(CF_UNICODETEXT);
        if (hglobal) {
          LPWSTR lpstr = static_cast<LPWSTR>(GlobalLock(hglobal));
          if (lpstr) {
            len =
              WideCharToMultiByte(CP_UTF8, 0, lpstr, wcslen(lpstr),
                                  NULL, 0, NULL, NULL) + 1;
            GlobalUnlock(hglobal);
          }
        }
      }
      else if (IsClipboardFormatAvailable(CF_TEXT)) {
        HGLOBAL hglobal = GetClipboardData(CF_TEXT);
        if (hglobal) {
          LPSTR lpstr = static_cast<LPSTR>(GlobalLock(hglobal));
          if (lpstr) {
            len = strlen(lpstr) + 1;
            GlobalUnlock(hglobal);
          }
        }
      }
    }

    return len;
  }

};

} // namespace copypaste
