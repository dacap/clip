// Clip Library
// Copyright (c) 2018 David Capello

#include "clip.h"

#include <vector>
#include <algorithm>

#include "png.h"

namespace clip {
namespace x11 {

void write_data_fn(png_structp png, png_bytep buf, png_size_t len) {
  std::vector<uint8_t>& output = *(std::vector<uint8_t>*)png_get_io_ptr(png);
  const size_t i = output.size();
  output.resize(i+len);
  std::copy(buf, buf+len, output.begin()+i);
}

bool create_png(const image& image,
                std::vector<uint8_t>& output)
{
  png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                            nullptr, nullptr, nullptr);
  if (!png)
    return false;

  png_infop info = png_create_info_struct(png);
  if (!info) {
    png_destroy_write_struct(&png, nullptr);
    return false;
  }

  if (setjmp(png_jmpbuf(png))) {
    png_destroy_write_struct(&png, &info);
    return false;
  }

  png_set_write_fn(png,
                   (png_voidp)&output,
                   write_data_fn,
                   nullptr);    // No need for a flush function

  const image_spec& spec = image.spec();
  int color_type = (spec.alpha_mask ?
                    PNG_COLOR_TYPE_RGB_ALPHA:
                    PNG_COLOR_TYPE_RGB);

  png_set_IHDR(png, info,
               spec.width, spec.height, 8, color_type,
               PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
  png_write_info(png, info);
  png_set_packing(png);

  png_bytep row =
    (png_bytep)png_malloc(png, png_get_rowbytes(png, info));

  for (png_uint_32 y=0; y<spec.height; ++y) {
    const uint32_t* src =
      (const uint32_t*)(((const uint8_t*)image.data())
                        + y*spec.bytes_per_row);
    uint8_t* dst = row;
    unsigned int x, c;

    for (x=0; x<spec.width; x++) {
      c = *(src++);
      *(dst++) = (c & spec.red_mask  ) >> spec.red_shift;
      *(dst++) = (c & spec.green_mask) >> spec.green_shift;
      *(dst++) = (c & spec.blue_mask ) >> spec.blue_shift;
      if (color_type == PNG_COLOR_TYPE_RGB_ALPHA)
        *(dst++) = (c & spec.alpha_mask) >> spec.alpha_shift;
    }

    png_write_rows(png, &row, 1);
  }

  png_free(png, row);
  png_write_end(png, info);
  png_destroy_write_struct(&png, &info);
  return true;
}

} // namespace x11
} // namespace clip
