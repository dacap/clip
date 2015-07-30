# Copy-Paste Library
*Copyright (C) 2015 David Capello*

> Distributed under [MIT license](LICENSE.txt)

Library to copy/retrieve content to/from the clipboard/pasteboard.

## Features

* Copy/paste UTF-8 text on Windows and Mac OS X.

## Example

    #include "copypaste.h"
    #include <iostream>

    int main() {
      copypaste::set_text("Hello World");

      std::string value;
      copypaste::get_text(value);
      std::cout << value << "\n";
    }

## To-do

* Support Linux (GTK? KDE? X11?) clipboard
* Add support to copy/paste images
* Add support to copy/paste user-defined formats
