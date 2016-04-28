// Clip Library
// Copyright (c) 2015-2016 David Capello

#include "clip.h"
#include <iostream>

using namespace clip;

int main() {
  if (has(image_format())) {
    image img;
    get_image(img);

    std::cout << "Clipboard image is " << img.width() << "x" << img.height() << "\n";
  }
  else {
    std::cout << "Clipboard doesn't contain an image\n";
  }
}
