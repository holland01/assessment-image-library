#pragma once

#include "def.h"
#include <glm/glm.hpp>

//-------------------------------------------------------------------------------------------------------
// plane
//-------------------------------------------------------------------------------------------------------

struct plane
{
	static constexpr float THRESHOLD = 0.3f;

    float       mDistance;
    glm::vec3   mNormal;
    glm::vec3   mReferencePoint;

    plane( float d = 0.0f,
           glm::vec3 normal = glm::vec3( 0.0f ),
           glm::vec3 point = glm::vec3( 0.0f ) )
        : mDistance( d ),
          mNormal( std::move( normal ) ),
          mReferencePoint( std::move( point ) )
    {
    }

    bool similar_to( const plane& x ) const
    { return mDistance == x.mDistance && mNormal == x.mNormal; }

	bool has_point( const glm::vec3& p ) const;
};

INLINE bool plane::has_point( const glm::vec3& p ) const
{
	float result = glm::dot( mNormal, p );

	result -= mDistance;

	return glm::abs( result ) < THRESHOLD;
}

static INLINE glm::vec3 plane_project( const glm::vec3& origin, const plane& p )
{
    glm::vec3 originToP( p.mReferencePoint - origin );

    float dist = glm::dot( originToP, p.mNormal );

    return std::move( glm::vec3( origin + p.mNormal * dist ) );
}


