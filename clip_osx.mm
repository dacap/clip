// Clip Library
// Copyright (c) 2015-2016 David Capello

#include "clip.h"
#include "clip_osx.h"

#include <cassert>
#include <vector>
#include <map>

#include <CoreFoundation/CoreFoundation.h>
#include <Foundation/Foundation.h>
#include <AppKit/AppKit.h>

namespace clip {

namespace {

  format g_last_format = 100;
  std::map<std::string, format> g_name_to_format;
  std::map<format, std::string> g_format_to_name;

}

lock::impl::impl(void*) {
}

lock::impl::~impl() {
}

bool lock::impl::locked() const {
  return true;
}

bool lock::impl::clear() {
  NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
  [pasteboard clearContents];
  return true;
}

bool lock::impl::is_convertible(format f) const {
  NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
  NSString* result = nil;

  if (f == text_format()) {
    result = [pasteboard availableTypeFromArray:[NSArray arrayWithObject:NSPasteboardTypeString]];
  }
  else {
    auto it = g_format_to_name.find(f);
    if (it != g_format_to_name.end()) {
      const std::string& name = it->second;
      NSString* string = [[NSString alloc] initWithBytesNoCopy:(void*)name.c_str()
                                                        length:name.size()
                                                      encoding:NSUTF8StringEncoding
                                                  freeWhenDone:NO];
      result = [pasteboard availableTypeFromArray:[NSArray arrayWithObject:string]];
    }
  }

  return (result ? true: false);
}

bool lock::impl::set_data(format f, const char* buf, size_t len) {
  NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];

  if (f == text_format()) {
    NSString* string = [[NSString alloc] initWithBytesNoCopy:(void*)buf
                                                      length:len
                                                    encoding:NSUTF8StringEncoding
                                                freeWhenDone:NO];
    [pasteboard clearContents];
    [pasteboard setString:string forType:NSPasteboardTypeString];
    return true;
  }
  else {
    auto it = g_format_to_name.find(f);
    if (it != g_format_to_name.end()) {
      const std::string& formatName = it->second;
      NSString* typeString = [[NSString alloc]
                               initWithBytesNoCopy:(void*)formatName.c_str()
                                            length:formatName.size()
                                          encoding:NSUTF8StringEncoding
                                      freeWhenDone:NO];
      NSData* data = [NSData dataWithBytesNoCopy:(void*)buf
                                          length:len
                                      freeWhenDone:NO];

      [pasteboard clearContents];
      if ([pasteboard setData:data forType:typeString])
        return true;
    }
  }
  return false;
}

bool lock::impl::get_data(format f, char* buf, size_t len) const {
  assert(buf);
  if (!buf || !is_convertible(f))
    return false;

  NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];

  if (f == text_format()) {
    NSString* string = [pasteboard stringForType:NSPasteboardTypeString];
    int reqsize = [string lengthOfBytesUsingEncoding:NSUTF8StringEncoding]+1;

    assert(reqsize <= len);
    if (reqsize > len) {
      // Buffer is too small
      return false;
    }

    if (reqsize == 0)
      return true;

    memcpy(buf, [string UTF8String], reqsize);
    return true;
  }

  auto it = g_format_to_name.find(f);
  if (it == g_format_to_name.end())
    return false;

  const std::string& formatName = it->second;
  NSString* typeString =
    [[NSString alloc] initWithBytesNoCopy:(void*)formatName.c_str()
                                   length:formatName.size()
                                 encoding:NSUTF8StringEncoding
                             freeWhenDone:NO];

  NSData* data = [pasteboard dataForType:typeString];
  if (!data)
    return false;

  [data getBytes:buf length:len];
  return true;
}

size_t lock::impl::get_data_length(format f) const {
  size_t len = 0;

  if (f == text_format()) {
    NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
    NSString* string = [pasteboard stringForType:NSPasteboardTypeString];
    return [string lengthOfBytesUsingEncoding:NSUTF8StringEncoding]+1;
  }

  return len;
}

format register_format(const std::string& name) {
  // Check if the format is already registered
  auto it = g_name_to_format.find(name);
  if (it != g_name_to_format.end())
    return it->second;

  NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
  NSString* string = [[NSString alloc] initWithBytesNoCopy:(void*)name.c_str()
                                                    length:name.size()
                                                  encoding:NSUTF8StringEncoding
                                              freeWhenDone:NO];

  [pasteboard declareTypes:[NSArray arrayWithObject:string]
                     owner:NSApp];

  format new_format = g_last_format++;
  g_name_to_format[name] = new_format;
  g_format_to_name[new_format] = name;
  return new_format;
}

} // namespace clip
