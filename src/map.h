#pragma once

#include "def.h"
#include "geom.h"
#include "renderer.h"

namespace map {

struct area_t
{
	glm::mat4 model, world;
	std::vector< geom::bounding_box_t > boundsList;

	area_t( const glm::vec3& dims,
			const glm::mat4& model,
			const glm::mat4& world,
			const glm::vec3& origin,
			uint32_t count );

	area_t( area_t&& m );
};

struct generator_t
{
	std::vector< area_t > areas;

	generator_t( void );
};

} // namespace mapgen

