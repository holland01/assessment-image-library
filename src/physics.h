#pragma once

#include "def.h"
#include <glm/glm.hpp>
#include <string>
#include <memory>
#include <vector>

namespace phys {

const float INFINITE_MASS = 0.0f;

struct body_t
{
	float invMass;
	glm::vec3 position, velocity, forceAccum, initialForce;

	glm::mat3 orientation;

	body_t( void );

	void ApplyCollision( const glm::vec3& normal );
	void Integrate( float t );
	void Reset( void );
	std::string Info( void ) const;
};

struct world_t
{
	std::vector< std::unique_ptr< body_t > > bodies;

	float time;
	float dt;

	world_t( float time, float dt );

	void Update( void );
};

}

