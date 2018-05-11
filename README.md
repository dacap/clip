# Clip Library
*Copyright (C) 2015-2018 David Capello*

> Distributed under [MIT license](LICENSE.txt)

Library to copy/retrieve content to/from the clipboard/pasteboard.

## Features

* Copy/paste UTF-8 text on Windows, macOS, and Linux (X11).
* Copy/paste RGB/RGBA images on Windows and macOS. This library use non-premultiplied alpha RGB values.
* Copy/paste user-defined data on Windows and macOS.

## Example

    #include "clip.h"
    #include <iostream>

    int main() {
      clip::set_text("Hello World");

      std::string value;
      clip::get_text(value);
      std::cout << value << "\n";
    }

## User-defined clipboard formats

    #include "clip.h"

    int main() {
      clip::format my_format =
        clip::register_format("com.appname.FormatName");

      int value = 32;

      clip::lock l;
      l.clear();
      l.set_data(clip::text_format(), "Alternative text for value 32");
      l.set_data(my_format, &value, sizeof(int));
    }

* [Limited number of clipboard formats on Windows](http://blogs.msdn.com/b/oldnewthing/archive/2015/03/19/10601208.aspx)

## To-do

* Add support to copy/paste images on Linux
* Add support to copy/paste user-defined formats on Linux
* Support keeping Linux clipboard content alive even when the process is closed

## Who is using this library?

`clip` is being used in the following applications:

* [Aseprite](https://github.com/aseprite/aseprite)
