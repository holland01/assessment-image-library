#pragma once

#include "_geom_local.h"

//-------------------------------------------------------------------------------------------------------
// plane
//-------------------------------------------------------------------------------------------------------

#define PLANE_ASSERT assert( glm::length( mNormal ) == 1.0f && "Normal must be normalized" )
struct plane
{
	static constexpr float THRESHOLD = 0.3f;

    float       mDistance;
    glm::vec3   mNormal;
    glm::vec3   mReferencePoint;

    plane( float d = 0.0f,
		   const glm::vec3& normal = glm::vec3( 0.0f ),
		   const glm::vec3& point = glm::vec3( 0.0f ) )
        : mDistance( d ),
		  mNormal( normal ),
		  mReferencePoint( point )
    {
    }

	plane( const glm::vec3& normal, const glm::vec3& point )
		: mDistance( glm::dot( normal, point ) ),
		  mNormal( normal ),
		  mReferencePoint( point )

	{
		PLANE_ASSERT;
	}

    bool similar_to( const plane& x ) const
    { return mDistance == x.mDistance && mNormal == x.mNormal; }

	bool has_point( const glm::vec3& p ) const;

	glm::vec3 project( const glm::vec3& origin ) const;
};

INLINE bool plane::has_point( const glm::vec3& p ) const
{
	float result = glm::dot( mNormal, p );

	result -= mDistance;

	return glm::abs( result ) < THRESHOLD;
}

INLINE glm::vec3 plane::project( const glm::vec3& origin ) const
{
	glm::vec3 originToP( mReferencePoint - origin );

	float dist = glm::dot( originToP, mNormal );

	return std::move( glm::vec3( origin + mNormal * dist ) );
}


