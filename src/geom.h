#pragma once

#include "def.h"
#include <glm/glm.hpp>
#include <stdint.h>
#include <stdio.h>

namespace view {
    struct params_t;
}

namespace geom {

struct drawVertex_t
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
    glm::u8vec4 color;
};

struct triangle_t
{
    uint32_t vertices[ 3 ]; // indices which map to vertices within an arbitrary buffer
};

struct plane_t
{
    float       d;
    glm::vec3   normal;
};

struct plane_t;

//-------------------------------------------------------------------------------------------------------
// AABB
//-------------------------------------------------------------------------------------------------------

class AABB
{
public:

    enum face_t
    {
        FACE_TOP = 0,
        FACE_RIGHT,
        FACE_FRONT,
        FACE_LEFT,
        FACE_BACK,
        FACE_BOTTOM
    };

    AABB( void ); // Calls Empty() on default init

    AABB( const glm::vec3& max, const glm::vec3& min );

    AABB( const AABB& toCopy );

    ~AABB( void );

    AABB&       operator =( AABB toAssign );

    bool		Encloses( const AABB& box ) const;

    void        Add( const glm::vec3& p );

    void        Empty( void ); // Sets maxPoint to -pseudoInfinity, and minPoint to pseudoInfinity

    void        TransformTo( const AABB& box, const glm::mat4& transform ); // Finds the smallest AABB from a given transformation

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

    static void		FromTransform( AABB& box, const glm::mat4& transform );

    static void		FromPoints( AABB& box, const glm::vec3 p[], int32_t n );

    glm::vec3 maxPoint, minPoint;
};

INLINE glm::vec4 AABB::Corner4( int32_t index ) const
{
    return glm::vec4( Corner( index ), 1.0f );
}

INLINE bool	AABB::Encloses( const AABB& box ) const
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

INLINE bool	AABB::InXRange( const glm::vec3& v ) const
{
    return ( v.x <= maxPoint.x && v.x >= minPoint.x );
}

INLINE bool AABB::InYRange( const glm::vec3& v ) const
{
    return ( v.y <= maxPoint.y && v.y >= minPoint.y );
}

INLINE bool AABB::InZRange( const glm::vec3& v ) const
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

class Frustum
{
    plane_t     frustPlanes[ FRUST_NUM_PLANES ];

    mutable uint32_t acceptCount;

    mutable uint32_t rejectCount;

    glm::mat4 mvp;

    glm::vec4 CalcPlaneFromOrigin( const glm::vec4& position, const glm::vec4& origin );

public:

    Frustum( void );

    ~Frustum( void );

    void    Update( const view::params_t& params );

    void	PrintMetrics( void ) const;

    void	ResetMetrics( void ) const { rejectCount = 0; acceptCount = 0; }

    bool    IntersectsBox( const AABB& box ) const;
};

INLINE void Frustum::PrintMetrics( void ) const
{
    printf( "Reject Count: %iu; Accept Count: %iu\r", rejectCount, acceptCount );
}

}


