// Copy-Paste Library
// Copyright (c) 2015 David Capello

#include "copypaste.h"
#include <iostream>

int main() {
  if (copypaste::has(copypaste::text_format())) {
    std::string value;
    copypaste::get_text(value);

    std::cout << "Clipboard content is '" << value << "'\n";
  }
  else {
    std::cout << "Clipboard doesn't contain text\n";
  }
}
