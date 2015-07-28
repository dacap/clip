// Copy-Paste Library
// Copyright (c) 2015 David Capello

#ifndef COPYPASTE_H_INCLUDED
#define COPYPASTE_H_INCLUDED
#pragma once

#include <string>

namespace copypaste {

  // Clipboard format identifier.
  typedef size_t format;

  // Low-level API to lock the clipboard/pasteboard and modify it.
  class lock {
  public:
    lock(void* native_handle);
    ~lock();

    bool clear();

    // Returns true if the clipboard can be converted to the given
    // format.
    bool is_convertible(format f) const;
    bool set_data(format f, const char* buf, size_t len);
    bool get_data(format f, char* buf, size_t len) const;
    size_t get_data_length(format f) const;

  private:
    class impl;
    impl* p;
  };

  // This format is when the clipboard has no content.
  format empty_format();

  // When the clipboard has UTF8 text.
  format text_format();

  // Returns true if the clipboard has content of the given type.
  bool has(format f);

  // Clears the clipboard content.
  bool clear();

  // High-level API to put/get UTF8 text in/from the clipboard. These
  // functions returns false in case of error.
  bool set_text(const std::string& value);
  bool get_text(std::string& value);

} // namespace copypaste

#endif // COPYPASTE_H_INCLUDED
