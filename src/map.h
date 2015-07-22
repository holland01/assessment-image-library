#pragma once

#include "def.h"
#include "geom.h"
#include "renderer.h"

namespace map {

struct area_t
{
	glm::mat3 transform;
	glm::vec3 origin;
	std::vector< geom::aabb_t > boundsList;

	area_t( const glm::vec3& dims, const glm::mat3& transform, const glm::vec3& origin, uint32_t count );
};

} // namespace mapgen

