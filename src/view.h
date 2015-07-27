#pragma once

#include "def.h"
#include "geom.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <array>

namespace view {

struct params_t
{
    glm::vec3   forward;
    glm::vec3   up;
    glm::vec3   right;

    glm::vec3   origin;

	glm::vec3   currRot, lastRot;

	glm::vec3   lastMouse;

    float       fovy, aspect, zNear, zFar;
    float		width, height;
	float		moveStep;

    glm::mat4   transform;

    glm::mat4   orientation;
    glm::mat4   inverseOrient;

    glm::mat4   clipTransform;

    params_t( void );
};


//-------------------------------------------------------------------------------------------------------
// Frustum
//-------------------------------------------------------------------------------------------------------

#define FRUST_NUM_PLANES 6

enum
{
	FRUST_NONE      = 6,
	FRUST_TOP       = 0,
	FRUST_BOTTOM    = 1,
	FRUST_RIGHT     = 2,
	FRUST_LEFT      = 3,
	FRUST_NEAR      = 4,
	FRUST_FAR       = 5
};

struct frustum_t
{
	geom::plane_t    frustPlanes[ FRUST_NUM_PLANES ];

	mutable uint32_t acceptCount;

	mutable uint32_t rejectCount;

	glm::mat4 mvp;

	glm::vec4 CalcPlaneFromOrigin( const glm::vec4& position, const glm::vec4& origin );

	frustum_t( void );

	~frustum_t( void );

	void    Update( const params_t& params );

	void	PrintMetrics( void ) const;

	void	ResetMetrics( void ) const { rejectCount = 0; acceptCount = 0; }

	bool    IntersectsBox( const geom::bounding_box_t& box ) const;
};

INLINE void frustum_t::PrintMetrics( void ) const
{
	printf( "Reject Count: %iu; Accept Count: %iu\r", rejectCount, acceptCount );
}

} // namespace view
