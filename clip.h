// Clip Library
// Copyright (c) 2015 David Capello

#ifndef CLIP_H_INCLUDED
#define CLIP_H_INCLUDED
#pragma once

#include <string>

namespace clip {

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

  format register_format(const std::string& name);

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

  // Error handling

  enum ErrorCode {
    CannotLock,
  };

  typedef void (*error_handler)(ErrorCode code);

  void set_error_handler(error_handler f);
  error_handler get_error_handler();

} // namespace clip

#endif // CLIP_H_INCLUDED
