#pragma once

#include "def.h"
#include <glm/glm.hpp>
#include <string>

namespace phys {

const float INFINITE_MASS = 0.0f;

struct body_t
{
	float invMass;
	glm::vec3 position, velocity, forceAccum, initialForce;

	glm::mat3 orientation;

	body_t( void );

	void Integrate( float t );
	void Reset( void );
	std::string Info( void ) const;
};

}

