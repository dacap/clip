project("clip")
    language("C++")
    cppdialect("C++17")
    filename("clip")
    kind("StaticLib")
    targetdir("bin/%{cfg.platform}/%{cfg.buildcfg}")

    files({"clip.h", "clip.cpp", "image.cpp", "clip_x11.cpp", "project.lua", ".gitignore"})
