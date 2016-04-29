// Clip Library
// Copyright (c) 2015-2016 David Capello

#include "clip.h"
#include <iostream>
#include <iomanip>

using namespace clip;

template<typename T, int w>
static void print_samples(const image& img,
                          const image_spec& spec) {
  std::cout << "Red:\n";
  for (int y=0; y<spec.height; ++y) {
    const T* p = (const T*)((char*)img.data()+spec.bytes_per_row*y);
    std::cout << "  ";
    for (int x=0; x<spec.width; ++x, ++p) {
      std::cout << std::right << std::hex << std::setw(w) << (((*p) & spec.red_mask)>>spec.red_shift) << " ";
    }
    std::cout << "\n";
  }

  std::cout << "Green:\n";
  for (int y=0; y<spec.height; ++y) {
    const T* p = (const T*)((char*)img.data()+spec.bytes_per_row*y);
    std::cout << "  ";
    for (int x=0; x<spec.width; ++x, ++p) {
      std::cout << std::right << std::hex << std::setw(w) << (((*p) & spec.green_mask)>>spec.green_shift) << " ";
    }
    std::cout << "\n";
  }

  std::cout << "Blue:\n";
  for (int y=0; y<spec.height; ++y) {
    const T* p = (const T*)((char*)img.data()+spec.bytes_per_row*y);
    std::cout << "  ";
    for (int x=0; x<spec.width; ++x, ++p) {
      std::cout << std::right << std::hex << std::setw(w) << (((*p) & spec.blue_mask)>>spec.blue_shift) << " ";
    }
    std::cout << "\n";
  }

  std::cout << "Alpha:\n";
  for (int y=0; y<spec.height; ++y) {
    const T* p = (const T*)((char*)img.data()+spec.bytes_per_row*y);
    std::cout << "  ";
    for (int x=0; x<spec.width; ++x, ++p) {
      std::cout << std::right << std::hex << std::setw(w) << (((*p) & spec.alpha_mask)>>spec.alpha_shift) << " ";
    }
    std::cout << "\n";
  }
}

int main() {
  if (has(image_format())) {
    image img;
    if (!get_image(img)) {
      std::cout << "Error getting image from clipboard\n";
      return 1;
    }

    image_spec spec = img.spec();

    std::cout << "Image in clipboard "
              << spec.width << "x" << spec.height
              << " (" << spec.bits_per_pixel << "bpp)\n"
              << "Format:" << "\n"
              << std::hex
              << "  Red   mask: " << spec.red_mask << "\n"
              << "  Green mask: " << spec.green_mask << "\n"
              << "  Blue  mask: " << spec.blue_mask << "\n"
              << "  Alpha mask: " << spec.alpha_mask << "\n"
              << std::dec
              << "  Red   shift: " << spec.red_shift << "\n"
              << "  Green shift: " << spec.green_shift << "\n"
              << "  Blue  shift: " << spec.blue_shift << "\n"
              << "  Alpha shift: " << spec.alpha_shift << "\n"
              << "Bytes:\n";

    std::cout << "Memory:\n";
    for (int y=0; y<spec.height; ++y) {
      char* p = img.data()+spec.bytes_per_row*y;
      std::cout << "  ";
      for (int x=0; x<spec.width; ++x) {
        for (int byte=0; byte<spec.bits_per_pixel/8; ++byte, ++p)
          std::cout << std::right << std::hex << std::setfill('0') << std::setw(2) << int((*p) & 0xff) << " ";
      }
      std::cout << "\n";
    }

    switch (spec.bits_per_pixel) {
      case 16: print_samples<uint16_t, 2>(img, spec); break;
      case 32: print_samples<uint32_t, 2>(img, spec); break;
      case 64: print_samples<uint64_t, 4>(img, spec); break;
    }
  }
  else {
    std::cout << "Clipboard doesn't contain an image\n";
  }
}
