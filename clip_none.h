// Clip Library
// Copyright (c) 2015 David Capello

#include <cassert>
#include <vector>

namespace clip {

static format g_format;
static std::vector<char> g_data;

class lock::impl {
public:
  impl(void* native_handle) {
  }

  bool clear() {
    g_format = empty_format();
    return true;
  }

  bool is_convertible(format f) const {
    return (g_format == f);
  }

  bool set_data(format f, const char* buf, size_t len) {
    g_format = f;
    g_data.resize(len);
    if (buf && len > 0)
      std::copy(buf, buf+len, g_data.begin());
    return true;
  }

  bool get_data(format f, char* buf) const {
    assert(buf);

    if (!buf || !is_convertible(f))
      return false;

    std::copy(g_data.begin(), g_data.end(), buf);
    return true;
  }

  size_t get_data_length(format f) const {
    if (is_convertible(f))
      return g_data.size();
    else
      return 0;
  }

};

} // namespace clip
