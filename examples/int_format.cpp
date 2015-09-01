// Copy-Paste Library
// Copyright (c) 2015 David Capello

#include "copypaste.h"
#include "random.h"
#include <cassert>
#include <iostream>

using namespace copypaste;

int main() {
  format int_format = register_format("CopyPaste.CustomInt");

  {
    lock l(nullptr);
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
    lock l(nullptr);
    l.clear();
    l.set_data(int_format, (const char*)&newData, sizeof(int));
  }

  {
    lock l(nullptr);

    int data = 0;
    l.get_data(int_format, (char*)&data, sizeof(int));

    assert(data == newData);

    std::cout << "New custom data in clipboard: " << data << "\n";
  }
}
