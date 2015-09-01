// Clip Library
// Copyright (c) 2015 David Capello

#include "clip.h"
#include "clip_osx.h"

#include <cassert>
#include <vector>

#include <CoreFoundation/CoreFoundation.h>
#include <Foundation/Foundation.h>
#include <AppKit/AppKit.h>

namespace clip {

lock::impl::impl(void*) {
}

lock::impl::~impl() {
}

bool lock::impl::clear() {
  NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
  [pasteboard clearContents];
  return true;
}

bool lock::impl::is_convertible(format f) const {
  return true;               // TODO
}

bool lock::impl::set_data(format f, const char* buf, size_t len) {
  if (f == text_format()) {
    NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];

    // TODO check "len" parameter

    [pasteboard clearContents];
    [pasteboard setString:[NSString stringWithUTF8String:buf]
                  forType:NSStringPboardType];
    return true;
  }
  else
    return false;
}

bool lock::impl::get_data(format f, char* buf, size_t len) const {
  assert(buf);
  if (!buf || !is_convertible(f))
    return false;

  if (f == text_format()) {
    NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
    NSString* string = [pasteboard stringForType:NSStringPboardType];
    int reqsize = [string length]+1;

    assert(reqsize <= len);
    if (reqsize > len) {
      // Buffer is too small
      return false;
    }

    memcpy(buf, [string UTF8String], reqsize);
    return true;
  }

  return false;
}

size_t lock::impl::get_data_length(format f) const {
  size_t len = 0;

  if (f == text_format()) {
    NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
    NSString* string = [pasteboard stringForType:NSStringPboardType];
    return [string length]+1;
  }

  return len;
}

} // namespace clip
