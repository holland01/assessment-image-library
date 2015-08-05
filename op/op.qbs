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
        "-Wno-unused-result",
        "-Wno-strict-aliasing",
        "-std=c++1y"
    ]
    cpp.linkerFlags: [
        "-L" + qbs.getEnv("DEVLIB_ROOT") + "/sdl2/lib/x86_64-linux-gnu/",
        "-L/usr/lib/x86_64-linux-gnu",
        "-lGL",
        "-lGLU",
        "-lGLEW",
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
        "../asset/shader/desktop/billboard.vert",
        "../asset/shader/desktop/billboard.frag",
        "../asset/shader/desktop/single_color.vert",
        "../asset/shader/desktop/single_color.frag",
        "../asset/shader/desktop/single_color_ss.frag",
        "../asset/shader/desktop/single_color_ss.vert",
        "../asset/shader/es/billboard.frag",
        "../asset/shader/es/billboard.vert",
        "../asset/shader/es/single_color.vert",
        "../asset/shader/es/single_color.frag",
        "../asset/shader/es/single_color_ss.frag",
        "../asset/shader/es/single_color_ss.vert",
        "../src/base.cpp",
        "../src/base.h",
        "../src/base.inl",
        "../src/def.h",
        "../src/eminput.h",
        "../src/entity.cpp",
        "../src/entity.h",
        "../src/game.h",
        "../src/geom.cpp",
        "../src/geom.h",
        "../src/input.cpp",
        "../src/input.h",
        "../src/lib/stb_image.c",
        "../src/main.cpp",
        "../src/map.cpp",
        "../src/map.h",
        "../src/opengl.h",
        "../src/physics.cpp",
        "../src/physics.h",
        "../src/physics.inl",
        "../src/renderer.cpp",
        "../src/renderer.h",
        "../src/renderer.inl",
        "../src/view.cpp",
        "../src/view.h",
    ]

    Group {
        condition: qbs.buildVariant == "debug"
        cpp.defines: outer.concat(["DEBUG"])
    }

    Group {     // Properties for the produced executable
        fileTagsFilter: product.type
        qbs.install: true
    }

    Group {
        files: [ "../asset/*" ]
        qbs.installDir: "asset"
        qbs.install: true
    }

    Properties {
        condition: cpp.compilerName == "emcc_gcc"
        cpp.defines: outer.concat(["EMSCRIPTEN"])
    }
}

