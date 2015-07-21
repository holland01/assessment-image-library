#pragma once

#include <SDL2/SDL.h>
#include <stdint.h>

enum class input_key_t : uint32_t
{
#ifdef EMSCRIPTEN
    ESC = 27,
    W = 87,
    S = 83,
    A = 65,
    D = 68,
    SPACE = 32,
    LSHIFT = 16,
    E = 69,
    Q = 81
#else
    ESC = SDLK_ESCAPE,
    W = SDLK_w,
    S = SDLK_s,
    A = SDLK_a,
    D = SDLK_d,
    SPACE = SDLK_SPACE,
    LSHIFT = SDLK_LSHIFT,
    E = SDLK_e,
    Q = SDLK_q
#endif
};

