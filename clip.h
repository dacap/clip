// Clip Library
// Copyright (c) 2015-2016 David Capello

#ifndef CLIP_H_INCLUDED
#define CLIP_H_INCLUDED
#pragma once

#include <cassert>
#include <string>

namespace clip {

  // ======================================================================
  // Low-level API to lock the clipboard/pasteboard and modify it
  // ======================================================================

  // Clipboard format identifier.
  typedef size_t format;

  class image;

  class lock {
  public:
    lock(void* native_handle);
    ~lock();

    // Returns true if we've locked the clipboard successfully in
    // lock() constructor.
    bool locked() const;

    // Clears the clipboard content. If you don't clear the content,
    // previous clipboard content (in unknown formats) could persist
    // after the unlock.
    bool clear();

    // Returns true if the clipboard can be converted to the given
    // format.
    bool is_convertible(format f) const;
    bool set_data(format f, const char* buf, size_t len);
    bool get_data(format f, char* buf, size_t len) const;
    size_t get_data_length(format f) const;

    // For images
    bool set_image(const image& image);
    bool get_image(image& image) const;

  private:
    class impl;
    impl* p;
  };

  format register_format(const std::string& name);

  // This format is when the clipboard has no content.
  format empty_format();

  // When the clipboard has UTF8 text.
  format text_format();

  // When the clipboard has an image.
  format image_format();

  // Returns true if the clipboard has content of the given type.
  bool has(format f);

  // Clears the clipboard content.
  bool clear();

  // ======================================================================
  // Error handling
  // ======================================================================

  enum ErrorCode {
    CannotLock,
  };

  typedef void (*error_handler)(ErrorCode code);

  void set_error_handler(error_handler f);
  error_handler get_error_handler();

  // ======================================================================
  // Text
  // ======================================================================

  // High-level API to put/get UTF8 text in/from the clipboard. These
  // functions returns false in case of error.
  bool set_text(const std::string& value);
  bool get_text(std::string& value);

  // ======================================================================
  // Image
  // ======================================================================

  class image {
  public:
    // Constructor to use get_image()
    image()
      : m_data(nullptr),
        m_width(0),
        m_height(0),
        m_rowstride(0) {
    }

    // Constructor to use set_image()
    image(char* data, int width, int height,
          int rowstride)
      : m_data(data),
        m_width(width),
        m_height(height),
        m_rowstride(rowstride) {
    }

    char* data() const { return m_data; }
    int width() const { return m_width; }
    int height() const { return m_height; }
    int rowstride() const { return m_rowstride; }

    template<typename T>
    T get_pixel(int x, int y) {
      assert(x >= 0 && x < m_width);
      assert(y >= 0 && y < m_height);
      return *(static_cast<T*>((static_cast<char*>(m_data)+m_rowstride*y))+x);
    }

    template<typename T>
    void put_pixel(int x, int y, T color) {
      assert(x >= 0 && x < m_width);
      assert(y >= 0 && y < m_height);
      *(static_cast<T*>((static_cast<char*>(m_data)+m_rowstride*y))+x) = color;
    }

  private:
    char* m_data;
    int m_width;
    int m_height;
    int m_rowstride;
  };

  // High-level API to set/get an image in/from the clipboard. These
  // functions returns false in case of error.
  bool set_image(const image& img);
  bool get_image(image& img);

} // namespace clip

#endif // CLIP_H_INCLUDED
