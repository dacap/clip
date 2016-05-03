// Clip Library
// Copyright (c) 2015-2016 David Capello

#include "clip.h"
#include "random.h"
#include <cassert>
#include <iostream>

int main() {
  clip::format int_format = clip::register_format("org.aseprite.clip.CustomInt");

  {
    clip::lock l(nullptr);
    if (l.is_convertible(int_format)) {
      int data = 0;
      if (l.get_data(int_format, (char*)&data, sizeof(int)))
        std::cout << "Existing custom data in clipboard: " << data << "\n";
    }
    else
      std::cout << "Clipboard doesn't have custom data\n";
  }

  int newData = RandomInt(0, 9999).generate();
  {
    clip::lock l(nullptr);
    l.clear();
    l.set_data(int_format, (const char*)&newData, sizeof(int));

    std::cout << "Set custom data in clipboard: " << newData << "\n";
  }

  {
    clip::lock l(nullptr);

    int data = 0;
    l.get_data(int_format, (char*)&data, sizeof(int));

    // This could fail if we are running several instances of this
    // example at the same time.
    assert(data == newData);

    std::cout << "Get custom data in clipboard: " << data << "\n";
  }
}
