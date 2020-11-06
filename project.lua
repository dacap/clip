project("clip")
    language("C++")
    cppdialect("C++17")
    filename("clip")
    kind("StaticLib")
    targetdir("bin/%{cfg.platform}/%{cfg.buildcfg}")

    buildoptions({"-w"})

    files({"clip.h", "clip.cpp", "image.cpp", "project.lua", ".gitignore"})

    filter("platforms:Linux*")
        files({"clip_x11.cpp", "clip_x11_png.h"})
