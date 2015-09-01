# Clip Library
*Copyright (C) 2015 David Capello*

> Distributed under [MIT license](LICENSE.txt)

Library to copy/retrieve content to/from the clipboard/pasteboard.

## Features

* Copy/paste UTF-8 text on Windows and Mac OS X.

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

    using namespace clip;

    int main() {
      format my_format = register_format("AppName.FormatName");

      int value = 32;

      lock l;
      l.clear();
      l.set_data(text_format(), "Alternative text for value 32");
      l.set_data(my_format, &value, sizeof(int));
    }

* [Limited number of clipboard formats on Windows](http://blogs.msdn.com/b/oldnewthing/archive/2015/03/19/10601208.aspx)

## To-do

* Support Linux (GTK? KDE? X11?) clipboard
* Add support to copy/paste images
* Add support to copy/paste user-defined formats on OS X and Linux
