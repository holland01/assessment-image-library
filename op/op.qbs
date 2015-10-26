import qbs

Product {
    type: "application" // To suppress bundle generation on Mac
    Depends { name: "cpp" }
    consoleApplication: true
    cpp.includePaths: {
        var inc = [
            "/usr/include/",
            "/usr/include/c++",
            "../src/lib",
            "../src"
        ];

        var librootInc = [
            "/sdl2/include",
            "/bullet3/",
            "/bullet3/bullet3",
          //  "/bullet3/bullet3/BulletCollision",
          //  "/bullet3/bullet3/BulletDynamics",
          //  "/bullet3/bullet3/LinearMath"
        ];

        var devlibRoot = qbs.getEnv("DEVLIB_ROOT");

        librootInc.forEach(function(includePath) {
            inc.push(devlibRoot + includePath);
        });

        return inc;
    }
    cpp.defines: ["__DEBUG_RENDERER__", "DEBUG"]
    cpp.treatWarningsAsErrors: true
    cpp.cxxFlags:[
        "-Wno-unused-function",
        "-Wno-unused-but-set-variable",
        "-Wno-unused-result",
        "-Wno-strict-aliasing",
        "-std=c++1y"
    ]
    cpp.linkerFlags: {
        var linkFlags = [
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
        ];

        linkFlags.push("-L" + qbs.getEnv("DEVLIB_ROOT") + "/bullet3/bin");

        var bulletSuffix = "_gmake_x64_debug";

        var bulletLibs = [
            "-lBulletDynamics",
            "-lBulletCollision",
            "-lLinearMath",
        ];

        bulletLibs.forEach(function(lib) {
            linkFlags.push(lib + bulletSuffix);
        });

        return linkFlags;
    }
    files: [
        "../Makefile",
        "../asset/shader/es/billboard.frag",
        "../asset/shader/es/billboard.vert",
        "../asset/shader/es/single_color.vert",
        "../asset/shader/es/single_color.frag",
        "../asset/shader/es/single_color_ss.frag",
        "../asset/shader/es/single_color_ss.vert",
        "../asset/shader/es/vertex_color.frag",
        "../asset/shader/es/vertex_color.vert",
        "../src/application.cpp",
        "../src/application.h",
        "../src/base.cpp",
        "../src/base.h",
        "../src/base.inl",
        "../src/bullet_ext.hpp",
        "../src/current.todo",
        "../src/debug.cpp",
        "../src/debug.h",
        "../src/debug.inl",
        "../src/debug_app.h",
        "../src/def.h",
        "../src/eminput.h",
        "../src/entity.cpp",
        "../src/entity.h",
        "../src/geom/_geom_local.cpp",
        "../src/geom/_geom_local.h",
        "../src/geom/geom.h",
        "../src/geom/geom_util.cpp",
        "../src/geom/geom_util.h",
        "../src/geom/halfspace.cpp",
        "../src/geom/halfspace.h",
        "../src/geom/plane.h",
        "../src/geom/point_project_pair.h",
        "../src/geom/ray.cpp",
        "../src/geom/ray.h",
        "../src/geom/transform_data.h",
        "../src/glm_ext.hpp",
        "../src/input.cpp",
        "../src/input.h",
        "../src/lib/stb_image.c",
        "../src/main.cpp",
        "../src/map.inl",
        "../src/messenger.h",
        "../src/opengl.h",
        "../src/physics/physics_body.cpp",
        "../src/physics/physics_body.h",
        "../src/physics/physics_entity.cpp",
        "../src/physics/physics_entity.h",
        "../src/physics/physics_world.cpp",
        "../src/physics/physics_world.h",
        "../src/procedural.cpp",
        "../src/procedural.h",
        "../src/renderer.cpp",
        "../src/renderer.h",
        "../src/renderer.inl",
        "../src/test/delaunay.cpp",
        "../src/test/delaunay.h",
        "../src/test/game.cpp",
        "../src/test/game.h",
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

    Group {
        files: [ "../asset/shader/desktop/*" ]
        qbs.installDir: "asset/shader/desktop"
        qbs.install: true
    }

    Properties {
        condition: cpp.compilerName == "emcc_gcc"
        cpp.defines: outer.concat(["EMSCRIPTEN"])
    }
}

