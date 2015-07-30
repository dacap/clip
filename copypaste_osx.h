// Copy-Paste Library
// Copyright (c) 2015 David Capello

#ifndef COPYPASTE_OSX_H_INCLUDED
#define COPYPASTE_OSX_H_INCLUDED

namespace copypaste {

class lock::impl {
public:
  impl(void*);
  ~impl();

  bool clear();
  bool is_convertible(format f) const;
  bool set_data(format f, const char* buf, size_t len);
  bool get_data(format f, char* buf, size_t len) const;
  size_t get_data_length(format f) const;
};

} // namespace copypaste

#endif
