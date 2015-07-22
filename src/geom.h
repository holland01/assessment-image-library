#pragma once

#include "def.h"
#include "renderer.h"
#include <glm/glm.hpp>
#include <stdint.h>
#include <stdio.h>
#include <memory>

namespace view {
    struct params_t;
}

namespace geom {

struct plane_t
{
    float       d;
    glm::vec3   normal;
};

struct plane_t;

//-------------------------------------------------------------------------------------------------------
// AABB
//-------------------------------------------------------------------------------------------------------

class aabb_t
{
public:

	using draw_t = rend::draw_buffer_t< 8 >;

	std::unique_ptr< draw_t > drawBuffer; // optional, use aabb_t::SetDrawable to initialize

    enum face_t
    {
        FACE_TOP = 0,
        FACE_RIGHT,
        FACE_FRONT,
        FACE_LEFT,
        FACE_BACK,
        FACE_BOTTOM
    };

    aabb_t( void ); // Calls Empty() on default init

    aabb_t( const glm::vec3& max, const glm::vec3& min );

	aabb_t( aabb_t&& toMove );

    ~aabb_t( void );

	// We make this movable only because of the drawBuffer member
	aabb_t( const aabb_t& toCopy ) = delete;
	aabb_t&      operator =( aabb_t toAssign ) = delete;

	aabb_t&		 operator =( aabb_t&& toMove );

    bool		Encloses( const aabb_t& box ) const;

    void        Add( const glm::vec3& p );

    void        Empty( void ); // Sets maxPoint to -pseudoInfinity, and minPoint to pseudoInfinity

    void        TransformTo( const aabb_t& box, const glm::mat4& transform ); // Finds the smallest AABB from a given transformation

    glm::vec3       GetMaxRelativeToNormal( const glm::vec3& normal ) const;

    glm::vec3       GetMinRelativeToNormal( const glm::vec3 &normal ) const;

    glm::vec3        Center( void ) const;

    glm::vec3        Size( void ) const;

    glm::vec3        Radius( void ) const;

    glm::vec3        Corner( int index ) const;

    glm::vec4		Corner4( int index ) const;

    bool			InXRange( const glm::vec3& v ) const;

    bool			InYRange( const glm::vec3& v ) const;

    bool			InZRange( const glm::vec3& v ) const;

    bool			IsEmpty( void ) const;

    bool			InPointRange( float k ) const;

    float			CalcIntersection( const glm::vec3& ray, const glm::vec3& origin ) const;

    void			GetFacePlane( face_t face, plane_t& plane ) const;

	void			SetDrawable( const glm::u8vec4& color );

    static void		FromTransform( aabb_t& box, const glm::mat4& transform );

    static void		FromPoints( aabb_t& box, const glm::vec3 p[], int32_t n );

    glm::vec3 maxPoint, minPoint;
};

INLINE aabb_t& aabb_t::operator= ( aabb_t&& toMove )
{
	maxPoint = std::move( toMove.maxPoint );
	minPoint = std::move( toMove.minPoint );
	drawBuffer = std::move( toMove.drawBuffer );

	return *this;
}

INLINE glm::vec4 aabb_t::Corner4( int32_t index ) const
{
    return glm::vec4( Corner( index ), 1.0f );
}

INLINE bool	aabb_t::Encloses( const aabb_t& box ) const
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

INLINE bool	aabb_t::InXRange( const glm::vec3& v ) const
{
    return ( v.x <= maxPoint.x && v.x >= minPoint.x );
}

INLINE bool aabb_t::InYRange( const glm::vec3& v ) const
{
    return ( v.y <= maxPoint.y && v.y >= minPoint.y );
}

INLINE bool aabb_t::InZRange( const glm::vec3& v ) const
{
#ifdef AABB_MAX_Z_LESS_THAN_MIN_Z
    return ( v.z >= maxPoint.z && v.z <= minPoint.z );
#else
    return ( v.z <= maxPoint.z && v.z >= minPoint.z );
#endif
}


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

class frustum_t
{
    plane_t     frustPlanes[ FRUST_NUM_PLANES ];

    mutable uint32_t acceptCount;

    mutable uint32_t rejectCount;

    glm::mat4 mvp;

    glm::vec4 CalcPlaneFromOrigin( const glm::vec4& position, const glm::vec4& origin );

public:

    frustum_t( void );

    ~frustum_t( void );

    void    Update( const view::params_t& params );

    void	PrintMetrics( void ) const;

    void	ResetMetrics( void ) const { rejectCount = 0; acceptCount = 0; }

    bool    IntersectsBox( const aabb_t& box ) const;
};

INLINE void frustum_t::PrintMetrics( void ) const
{
    printf( "Reject Count: %iu; Accept Count: %iu\r", rejectCount, acceptCount );
}

}


