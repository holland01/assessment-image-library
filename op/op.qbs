import qbs

Product {
    type: "application" // To suppress bundle generation on Mac
    Depends { name: "cpp" }
    consoleApplication: true
    cpp.includePaths: [
        "/usr/include/",
        "/usr/include/c++",
        "../src/lib",
        qbs.getEnv("DEVLIB_ROOT") + "/sdl2/include"
    ]
    cpp.defines: ["__DEBUG_RENDERER__"]
    cpp.treatWarningsAsErrors: true
    cpp.cxxFlags:[
        "-Wno-unused-function",
        "-Wno-unused-but-set-variable",
        "-std=c++11"
    ]
    cpp.linkerFlags: [
        "-L" + qbs.getEnv("DEVLIB_ROOT") + "/sdl2/lib/x86_64-linux-gnu/",
        "-lGLESv2",
        "-L/usr/lib/x86_64-linux-gnu",
        "-lSDL2",
        "-lpthread",
        "-Wl,--no-undefined",
        "-lm",
        "-ldl",
        "-lasound",
        "-lm",
        "-ldl",
        "-lpthread",
        "-lpulse-simple",
        "-lpulse",
        "-lX11",
        "-lXext",
        "-lXcursor",
        "-lXinerama",
        "-lXi",
        "-lXrandr",
        "-lXss",
        "-lXxf86vm",
        "-lwayland-egl",
        "-lwayland-client",
        "-lwayland-cursor",
        "-lxkbcommon",
        "-lpthread",
        "-lrt"
    ]
    files: [
        "../Makefile",
        "../src/base.cpp",
        "../src/base.h",
        "../src/base.inl",
        "../src/def.h",
        "../src/geom.h",
        "../src/lib/stb_image.c",
        "../src/main.cpp",
        "../src/renderer.cpp",
        "../src/renderer.h",
        "../src/renderer.inl",
    ]

    Group {     // Properties for the produced executable
        fileTagsFilter: product.type
        qbs.install: true
    }
}

