#pragma once

#include <SDL2/SDL.h>
#include <stdint.h>

enum class input_key_t : uint32_t
{
    W = SDLK_w,
    S = SDLK_s,
    A = SDLK_a,
    D = SDLK_d,
    SPACE = SDLK_SPACE,
    LSHIFT = SDLK_LSHIFT,
    E = SDLK_e,
    Q = SDLK_q
};

