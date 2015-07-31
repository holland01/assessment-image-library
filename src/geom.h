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

struct ray_t
{
	glm::vec3 p;
	glm::vec3 d;
};

struct half_space_t
{
	glm::mat3 extents;
	glm::vec3 origin;
	float distance;

	half_space_t( void );

	bool TestBounds( glm::vec3& normal, const glm::mat3& extents, const glm::vec3& origin ) const;
	void Draw( rend::imm_draw_t& drawer ) const;
};

using point_predicate_t = bool ( * )( float );

template < size_t N, point_predicate_t predicate >
static INLINE bool PointPlaneTest( const std::array< glm::vec3, N >& points, const plane_t& plane )
{
    for ( const glm::vec3& p: points )
    {
        float x = glm::dot( p, plane.normal ) - plane.d;

        if ( ( *predicate )( x ) )
        {
            return true;
        }
    }

    return false;
}

static INLINE float TripleProduct( const glm::vec3& a, const glm::vec3& b, const glm::vec3& c )
{
	return glm::dot( a, glm::cross( b, c ) );
}

glm::vec3 PlaneProject( const glm::vec3& p, const glm::vec3& origin, const glm::vec3& normal );

bool RayRayTest( const ray_t& r0, const ray_t& r1, float& t0, float& t1 );

//-------------------------------------------------------------------------------------------------------
// bounding_box_t
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

	enum corner_t
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

	bool			Encloses( const bounding_box_t& box ) const;

	void			Add( const glm::vec3& p );

	void			Empty( void ); // Sets maxPoint to -pseudoInfinity, and minPoint to pseudoInfinity

	void			TransformTo( const bounding_box_t& box, const glm::mat4& transform ); // Finds the smallest AABB from a given transformation

    glm::vec3       GetMaxRelativeToNormal( const glm::vec3& normal ) const;

    glm::vec3       GetMinRelativeToNormal( const glm::vec3 &normal ) const;

	glm::vec3       GetCenter( void ) const;

	glm::vec3       GetSize( void ) const;

	glm::vec3       GetRadius( void ) const;

	glm::vec3       GetCorner( corner_t index ) const;

	glm::vec3       GetCornerIdentity( corner_t index ) const;

	void			GetEdgesFromCorner( corner_t index, glm::mat3& edges ) const;

    bool			InXRange( const glm::vec3& v ) const;

    bool			InYRange( const glm::vec3& v ) const;

    bool			InZRange( const glm::vec3& v ) const;

	bool			EnclosesPoint( const glm::vec3& v ) const;

    bool			IsEmpty( void ) const;

    bool			InPointRange( float k ) const;

	bool			IntersectsHalfSpace( glm::vec3& normal, const half_space_t& halfSpace ) const;

    float			CalcIntersection( const glm::vec3& ray, const glm::vec3& origin ) const;

    void			GetFacePlane( face_t face, plane_t& plane ) const;

    void			SetDrawable( const glm::vec4& color );

    void            GetPoints( std::array< glm::vec3, 8 >& points ) const;

	static void		FromTransform( bounding_box_t& box, const glm::mat4& transform );

	static void		FromPoints( bounding_box_t& box, const glm::vec3 p[], int32_t n );
};

INLINE glm::vec3 bounding_box_t::GetCornerIdentity( corner_t index ) const
{
	return glm::vec3(
		( ( int32_t ) index & 1 ) ? 1.0f : -1.0f,
		( ( int32_t ) index & 2 ) ? 1.0f : -1.0f,
		( ( int32_t ) index & 4 ) ? 1.0f : -1.0f
	);
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
	if ( oriented )
	{
		return v.x <= GetCorner( CORNER_MAX ).x && v.x >= GetCorner( CORNER_MIN ).x;
	}
    return ( v.x <= maxPoint.x && v.x >= minPoint.x );
}

INLINE bool bounding_box_t::InYRange( const glm::vec3& v ) const
{
	if ( oriented )
	{
		return v.y <= GetCorner( CORNER_MAX ).y && v.y >= GetCorner( CORNER_MIN ).y;
	}
    return ( v.y <= maxPoint.y && v.y >= minPoint.y );
}

INLINE bool bounding_box_t::InZRange( const glm::vec3& v ) const
{
#ifdef AABB_MAX_Z_LESS_THAN_MIN_Z
	if ( oriented )
	{
		return v.z >= GetCorner( CORNER_MAX ).z && v.z <= GetCorner( CORNER_MIN ).z;
	}
    return ( v.z >= maxPoint.z && v.z <= minPoint.z );
#else
	if ( oriented )
	{
		return v.z <= GetCorner( CORNER_MAX ).z && v.z >= GetCorner( CORNER_MIN ).z;
	}
    return ( v.z <= maxPoint.z && v.z >= minPoint.z );
#endif
}

INLINE bool bounding_box_t::EnclosesPoint( const glm::vec3& v ) const
{
	return InXRange( v ) && InYRange( v ) && InZRange( v );
}

INLINE void bounding_box_t::GetPoints( std::array< glm::vec3, 8 >& points ) const
{
	points[ 0 ] = GetCorner( ( corner_t ) 0 );
	points[ 1 ] = GetCorner( ( corner_t ) 1 );
	points[ 2 ] = GetCorner( ( corner_t ) 2 );
	points[ 3 ] = GetCorner( ( corner_t ) 3 );
	points[ 4 ] = GetCorner( ( corner_t ) 4 );
	points[ 5 ] = GetCorner( ( corner_t ) 5 );
	points[ 6 ] = GetCorner( ( corner_t ) 6 );
	points[ 7 ] = GetCorner( ( corner_t ) 7 );
}

}


