// Clip Library
// Copyright (c) 2015-2016 David Capello

#ifndef CLIP_OSX_H_INCLUDED
#define CLIP_OSX_H_INCLUDED

namespace clip {

class lock::impl {
public:
  impl(void*);
  ~impl();

  bool locked() const;
  bool clear();
  bool is_convertible(format f) const;
  bool set_data(format f, const char* buf, size_t len);
  bool get_data(format f, char* buf, size_t len) const;
  size_t get_data_length(format f) const;
};

} // namespace clip

#endif
