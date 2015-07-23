#pragma once

#include "def.h"
#include "geom.h"
#include "renderer.h"

namespace map {

struct area_t
{
	glm::mat4 transform;
	glm::vec3 origin;
	std::vector< geom::bounding_box_t > boundsList;

	area_t( const glm::vec3& dims, const glm::mat4& transform, const glm::vec3& origin, uint32_t count );
};

} // namespace mapgen

