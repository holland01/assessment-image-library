#pragma once

#include <glm/glm.hpp>

#include <stdint.h>

struct drawVertex_t
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
    glm::u8vec4 color;
};

struct triangle_t
{
    uint32_t indices[ 3 ];
};
