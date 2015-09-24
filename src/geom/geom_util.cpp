#include "geom_util.h"

bool test_ray_ray( const ray& r0, const ray& r1, float& t0, float& t1 )
{
	glm::vec3 crossDir( glm::cross( r0.d, r1.d ) );

	float dist = glm::distance( r0.p, r1.p );

	if ( crossDir == glm::zero< glm::vec3 >() && dist > DISTANCE_THRESH  )
	{
		return false;
	}

	glm::vec3 pointDiff( r0.p - r1.p );
	glm::vec3 pointDiffCross1( glm::cross( pointDiff, r1.d ) );

	float crossMag = glm::length( crossDir );
	float invMagSqr = 1.0f / ( crossMag * crossMag );

	t0 = glm::dot( pointDiffCross1, crossDir ) * invMagSqr;

	glm::vec3 pointDiffCross0( glm::cross( pointDiff, r0.d ) );
	t1 = glm::dot( pointDiffCross0, crossDir ) * invMagSqr;

	glm::vec3 pr0( r0.p + r0.d * t0 );
	glm::vec3 pr1( r1.p + r1.d * t1 );

	if ( glm::distance( pr0, pr1 ) > DISTANCE_THRESH )
	{
		// skew lines
		return false;
	}

	return true;
}
