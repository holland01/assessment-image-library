#pragma once

#include "def.h"
#include "renderer.h"
#include <glm/glm.hpp>
#include <stdint.h>
#include <stdio.h>
#include <memory>

namespace geom {

struct plane_t
{
    float       d;
    glm::vec3   normal;
};

//-------------------------------------------------------------------------------------------------------
// AABB
//-------------------------------------------------------------------------------------------------------

struct bounding_box_t
{
	glm::mat4 transform;
	bool oriented;
	glm::vec3 maxPoint, minPoint;
    glm::vec4 color;

    enum face_t
    {
        FACE_TOP = 0,
        FACE_RIGHT,
        FACE_FRONT,
        FACE_LEFT,
        FACE_BACK,
        FACE_BOTTOM
    };

	enum
	{
		CORNER_MIN = 0,
		CORNER_NEAR_DOWN_RIGHT,
		CORNER_NEAR_UP_LEFT,
		CORNER_NEAR_UP_RIGHT,
		CORNER_FAR_DOWN_LEFT,
		CORNER_FAR_DOWN_RIGHT,
		CORNER_FAR_UP_LEFT,
		CORNER_MAX = 7
	};

	bounding_box_t( void ); // Calls Empty() on default init

	bounding_box_t( const glm::vec3& max,
					const glm::vec3& min,
					const glm::mat4& transform = glm::mat4( 1.0f ),
					bool oriented = false );

	bounding_box_t( bounding_box_t&& m );

	// We make this movable only because of the drawBuffer member
	bounding_box_t( const bounding_box_t& toCopy ) = delete;
	bounding_box_t& operator =( bounding_box_t toAssign ) = delete;

	bool		Encloses( const bounding_box_t& box ) const;

    void        Add( const glm::vec3& p );

    void        Empty( void ); // Sets maxPoint to -pseudoInfinity, and minPoint to pseudoInfinity

	void        TransformTo( const bounding_box_t& box, const glm::mat4& transform ); // Finds the smallest AABB from a given transformation

    glm::vec3       GetMaxRelativeToNormal( const glm::vec3& normal ) const;

    glm::vec3       GetMinRelativeToNormal( const glm::vec3 &normal ) const;

    glm::vec3       Center( void ) const;

    glm::vec3       Size( void ) const;

    glm::vec3       Radius( void ) const;

    glm::vec3       Corner( int index ) const;

    glm::vec4       Corner4( int index ) const;

    bool			InXRange( const glm::vec3& v ) const;

    bool			InYRange( const glm::vec3& v ) const;

    bool			InZRange( const glm::vec3& v ) const;

    bool			IsEmpty( void ) const;

    bool			InPointRange( float k ) const;

    float			CalcIntersection( const glm::vec3& ray, const glm::vec3& origin ) const;

    void			GetFacePlane( face_t face, plane_t& plane ) const;

    void			SetDrawable( const glm::vec4& color );

	static void		FromTransform( bounding_box_t& box, const glm::mat4& transform );

	static void		FromPoints( bounding_box_t& box, const glm::vec3 p[], int32_t n );
};

INLINE glm::vec4 bounding_box_t::Corner4( int32_t index ) const
{
    return glm::vec4( Corner( index ), 1.0f );
}

INLINE bool	bounding_box_t::Encloses( const bounding_box_t& box ) const
{
#ifdef AABB_MAX_Z_LESS_THAN_MIN_Z

    if ( minPoint.x > box.minPoint.x ) return false;
    if ( maxPoint.x < box.maxPoint.x ) return false;

    if ( minPoint.y > box.minPoint.y ) return false;
    if ( maxPoint.y < box.maxPoint.y ) return false;

    if ( minPoint.z < box.minPoint.z ) return false;
    if ( maxPoint.z > box.maxPoint.z ) return false;

    return true;
#else
    return !glm::any( glm::greaterThan( minPoint, box.maxPoint ) ) && !glm::any( glm::lessThan( maxPoint, box.minPoint ) );
#endif
}

INLINE bool	bounding_box_t::InXRange( const glm::vec3& v ) const
{
    return ( v.x <= maxPoint.x && v.x >= minPoint.x );
}

INLINE bool bounding_box_t::InYRange( const glm::vec3& v ) const
{
    return ( v.y <= maxPoint.y && v.y >= minPoint.y );
}

INLINE bool bounding_box_t::InZRange( const glm::vec3& v ) const
{
#ifdef AABB_MAX_Z_LESS_THAN_MIN_Z
    return ( v.z >= maxPoint.z && v.z <= minPoint.z );
#else
    return ( v.z <= maxPoint.z && v.z >= minPoint.z );
#endif
}




}


