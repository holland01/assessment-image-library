#pragma once

#include "_geom_local.h"
#include "plane.h"
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

static const glm::vec3 G_DIR_RIGHT( 1.0f, 0.0f, 0.0f );
static const glm::vec3 G_DIR_UP( 0.0f, 1.0f, 0.0f );
static const glm::vec3 G_DIR_FORWARD( 0.0f, 0.0f, -1.0f );

const float DISTANCE_THRESH = 0.03125f;

// rightAxis can be thought of as an initial axis with which the angle will originate from,
// dir is the terminating axis ending the angle, and backAxis is the axis which is orthogonal to the rightAxis
// such that the angle between the two is 270 degrees, counter-clockwise.
// All are assumed to be normalized
static INLINE glm::mat3 orient_by_direction( const glm::vec3& dir, const glm::vec3& rightAxis, const glm::vec3& backAxis )
{
	float rot = glm::acos( glm::dot( rightAxis, dir ) );

	if ( glm::dot( backAxis, dir ) > 0.0f )
	{
		rot = -rot;
	}

	return std::move( glm::mat3( glm::rotate( glm::mat4( 1.0f ), rot, glm::vec3( 0.0f, 1.0f, 0.0f ) ) ) );
}

static INLINE glm::mat3 orient_to( const glm::vec3& srcPosition, const glm::vec3& destPosition, const glm::mat3& srcOrient )
{
	//glm::vec3 backOrtho( glm::cross( srcOrient[ 0 ], srcOrient[ 1 ] ) );

	return orient_by_direction( glm::normalize( destPosition - srcPosition ),
				glm::normalize( srcOrient[ 0 ] ), glm::normalize( srcOrient[ 2 ] ) );
}

static INLINE void rotate_matrix_xyz( glm::mat4& r, const glm::vec3& rotation )
{
	r = glm::rotate( glm::mat4( 1.0f ), rotation.x, glm::vec3( 1.0f, 0.0f, 0.0f ) );
	r = glm::rotate( r, rotation.y, glm::vec3( 0.0f, 1.0f, 0.0f ) );
	r = glm::rotate( r, rotation.z, glm::vec3( 0.0f, 0.0f, 1.0f ) );
}

using point_predicate_t = bool ( * )( float );

template < point_predicate_t predicate >
static INLINE bool test_point_plane( const std::vector< glm::vec3 >& points, const plane& pln )
{
	for ( const glm::vec3& p: points )
	{
		float x = glm::dot( p, pln.mNormal ) - pln.mDistance;

		if ( ( *predicate )( x ) )
		{
			return true;
		}
	}

	return false;
}

static INLINE float triple_product( const glm::vec3& a, const glm::vec3& b, const glm::vec3& c )
{
	return glm::dot( a, glm::cross( b, c ) );
}

bool test_ray_ray( const ray& r0, const ray& r1, float& t0, float& t1 );

