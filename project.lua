project("clip")
    language("C++")
    cppdialect("C++17")
    filename("clip")
    kind("StaticLib")
    targetdir("bin/%{cfg.platform}/%{cfg.buildcfg}")

    files({"clip.cpp", "image.cpp", "project.lua", ".gitignore"})
