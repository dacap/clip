// Copy-Paste Library
// Copyright (c) 2015 David Capello

#include "copypaste.h"

#ifdef _WIN32
  #include "copypaste_win.h"
#else
  #include "copypaste_none.h"
#endif

namespace copypaste {

lock::lock(void* native_handle)
  : p(new impl(native_handle)) {
}

lock::~lock() {
  delete p;
}

bool lock::clear() {
  return p->clear();
}

bool lock::is_convertible(format f) const {
  return p->is_convertible(f);
}

bool lock::set_data(format f, const char* buf, size_t length) {
  return p->set_data(f, buf, length);
}

bool lock::get_data(format f, char* buf, size_t len) const {
  return p->get_data(f, buf, len);
}

size_t lock::get_data_length(format f) const {
  return p->get_data_length(f);
}

format empty_format() {
  return 0;
}

format text_format() {
  return 1;
}

bool has(format f) {
  lock l(nullptr);
  return l.is_convertible(f);
}

bool clear() {
  lock l(nullptr);
  return l.clear();
}

bool set_text(const std::string& value) {
  lock l(nullptr);
  return l.set_data(text_format(), value.c_str(), value.size()+1);
}

bool get_text(std::string& value) {
  lock l(nullptr);
  format f = text_format();
  if (!l.is_convertible(f))
    return false;

  size_t len = l.get_data_length(f);
  if (len > 0) {
    std::vector<char> buf(len);
    l.get_data(f, &buf[0], len);
    value = &buf[0];
    return true;
  }
  else {
    value.clear();
    return true;
  }
}

} // namespace copypaste
