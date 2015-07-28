// Copy-Paste Library
// Copyright (c) 2015 David Capello

#include "copypaste.h"
#include <cassert>
#include <iostream>

int main() {
  copypaste::set_text("Hello World");

  std::string value;
  bool result = copypaste::get_text(value);

  assert(result);
  assert(copypaste::has(copypaste::text_format()));
  assert(value == "Hello World");

  std::cout << "'" << value << "' was copied to the clipboard\n";
}
