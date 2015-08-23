#pragma once

#include "def.h"
#include "renderer.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <stdint.h>
#include <stdio.h>
#include <memory>
#include <stack>

struct plane_t;
struct ray_t;

static const glm::vec3 G_DIR_RIGHT( 1.0f, 0.0f, 0.0f );
static const glm::vec3 G_DIR_UP( 0.0f, 1.0f, 0.0f );
static const glm::vec3 G_DIR_FORWARD( 0.0f, 0.0f, -1.0f );

//-------------------------------------------------------------------------------------------------------
// util
//-------------------------------------------------------------------------------------------------------

// rightAxis can be thought of as an initial axis with which the angle will originate from,
// dir is the terminating axis ending the angle, and backAxis is the axis which is orthogonal to the rightAxis
// such that the angle between the two is 270 degrees, counter-clockwise.
// All are assumed to be normalized
static INLINE glm::mat3 G_OrientByDirection( const glm::vec3& dir, const glm::vec3& rightAxis, const glm::vec3& backAxis );

static INLINE void G_RotateMatrixXYZ( glm::mat4& r, const glm::vec3& rotation );

using point_predicate_t = bool ( * )( float );
template < size_t N, point_predicate_t predicate >
static INLINE bool G_PointPlaneTest( const std::array< glm::vec3, N >& points, const plane_t& plane );

static INLINE float G_TripleProduct( const glm::vec3& a, const glm::vec3& b, const glm::vec3& c );

glm::vec3 G_PlaneProject( const glm::vec3& p, const glm::vec3& origin, const glm::vec3& normal );

bool G_RayRayTest( const ray_t& r0, const ray_t& r1, float& t0, float& t1 );


//-------------------------------------------------------------------------------------------------------
// plane_t
//-------------------------------------------------------------------------------------------------------

struct plane_t
{
    float       d;
    glm::vec3   normal;
};

//-------------------------------------------------------------------------------------------------------
// ray_t
//-------------------------------------------------------------------------------------------------------

struct ray_t
{
	glm::vec3 p;
	glm::vec3 d;
};

//-------------------------------------------------------------------------------------------------------
// transform_t
//-------------------------------------------------------------------------------------------------------

struct transform_t
{
protected:
    glm::vec3 scale;

    glm::vec3 rotation;

    glm::vec3 translation;

    glm::mat4 top;

    std::stack< glm::mat4 > matStack;

public:
    transform_t( void );

    void PushTransform( void );

    void PopTransform( void );

    const glm::mat4& PeekTransform( void ) const;

    void SetScale( const glm::vec3& s );

    void SetRotation( const glm::vec3& r );

    void SetTranslation( const glm::vec3& t );

    void ApplyScale( void );

    void ApplyRotation( void );

    void ApplyTranslation( void );

    void ApplyScale( const glm::vec3& s );

    void ApplyRotation( const glm::vec3& r );

    void ApplyTranslation( const glm::vec3& t );

    glm::mat3 GetScale( void ) const;

    glm::mat3 GetRotation3( void ) const;

    glm::mat4 GetTranslation( void ) const;
};

//-------------------------------------------------------------------------------------------------------
// bounds_primitive_t
//-------------------------------------------------------------------------------------------------------

enum bounds_primtype_t
{   BOUNDS_PRIM_HALFSPACE = 0,
    BOUNDS_PRIM_BOX,
    BOUNDS_PRIM_LOOKUP,
    NUM_BOUNDS_PRIMTYPE
};


struct primitive_lookup_t;
struct bounding_box_t;
struct half_space_t;

struct bounds_primitive_t
{
protected:
    bounds_primitive_t( bounds_primtype_t type_ )
        : type( type_ )
    {}

public:
    const bounds_primtype_t type;

    primitive_lookup_t*         ToLookup( void );

    const primitive_lookup_t*   ToLookup( void ) const;

    bounding_box_t*             ToBox( void );

    const bounding_box_t*       ToBox( void ) const;

    half_space_t*               ToHalfSpace( void );
};

//-------------------------------------------------------------------------------------------------------
// primitive_lookup_t
//-------------------------------------------------------------------------------------------------------

struct primitive_lookup_t : public bounds_primitive_t
{
    static const int32_t LOOKUP_UNSET = -1;

    bounds_primtype_t lookupType;
    int32_t index = 0;

    primitive_lookup_t( bounds_primtype_t lookupType_, int32_t index_ = LOOKUP_UNSET )
        : bounds_primitive_t( BOUNDS_PRIM_LOOKUP ),
          lookupType( lookupType_ ),
          index( index_ )
    {}
};

//-------------------------------------------------------------------------------------------------------
// half_space_t
//-------------------------------------------------------------------------------------------------------

struct bounding_box_t;

struct half_space_t : public bounds_primitive_t
{
	glm::mat3 extents;
	glm::vec3 origin;
	float distance;

    half_space_t( void );
    half_space_t( const glm::mat3& extents, const glm::vec3& origin, float distance );
    half_space_t( const bounding_box_t& bounds, const glm::vec3& normal );

    half_space_t( const half_space_t& c );
    half_space_t& operator=( half_space_t c );

	bool TestBounds( glm::vec3& normal, const glm::mat3& extents, const glm::vec3& origin ) const;
	void Draw( imm_draw_t& drawer ) const;
};

//-------------------------------------------------------------------------------------------------------
// bounding_box_t
//-------------------------------------------------------------------------------------------------------

struct bounding_box_t : public bounds_primitive_t
{
public:
	glm::mat4 transform;
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

    bounding_box_t( const glm::mat4& transform = glm::mat4( 1.0f ) );

    bounding_box_t( bounding_box_t&& m );

	// We make this movable only because of the drawBuffer member
    bounding_box_t( const bounding_box_t& toCopy ) = delete;
    bounding_box_t& operator =( bounding_box_t toAssign ) = delete;

    bool			Encloses( const bounding_box_t& box ) const;

	glm::vec3       GetCenter( void ) const;

	glm::vec3       GetSize( void ) const;

	glm::vec3       GetRadius( void ) const;

	glm::vec3       GetCorner( corner_t index ) const;

	glm::vec3       GetCornerIdentity( corner_t index ) const;

    const glm::mat4& GetTransform( void ) const;

	void			GetEdgesFromCorner( corner_t index, glm::mat3& edges ) const;

    void            GetPoints( std::array< glm::vec3, 8 >& points ) const;

    void			GetFacePlane( face_t face, plane_t& plane_t ) const;

    void			SetDrawable( const glm::vec4& color );

    void            SetCenter( const glm::vec3& position );

    void            SetOrientation( const glm::mat3& orient );

    const glm::vec4& operator[]( uint32_t i ) const;

    bool			InXRange( const glm::vec3& v ) const;

    bool			InYRange( const glm::vec3& v ) const;

    bool			InZRange( const glm::vec3& v ) const;

	bool			EnclosesPoint( const glm::vec3& v ) const;

    bool			IntersectsBounds( glm::vec3& normal, const bounding_box_t& bounds ) const;

    bool			IntersectsHalfSpace( glm::vec3& normal, const half_space_t& halfSpace ) const;

    bool			CalcIntersection( float& t0, const glm::vec3& ray_t, const glm::vec3& origin ) const;

};


#include "geom.inl"


//-------------------------------------------------------------------------------------------------------
// util
//-------------------------------------------------------------------------------------------------------

// rightAxis can be thought of as an initial axis with which the angle will originate from,
// dir is the terminating axis ending the angle, and backAxis is the axis which is orthogonal to the rightAxis
// such that the angle between the two is 270 degrees, counter-clockwise.
// All are assumed to be normalized
static INLINE glm::mat3 G_OrientByDirection( const glm::vec3& dir, const glm::vec3& rightAxis, const glm::vec3& backAxis )
{
    float rot = glm::acos( glm::dot( rightAxis, dir ) );

    if ( glm::dot( backAxis, dir ) > 0.0f )
    {
        rot = -rot;
    }

    return std::move( glm::mat3( glm::rotate( glm::mat4( 1.0f ), rot, glm::vec3( 0.0f, 1.0f, 0.0f ) ) ) );
}

static INLINE void G_RotateMatrixXYZ( glm::mat4& r, const glm::vec3& rotation )
{
    r = glm::rotate( glm::mat4( 1.0f ), rotation.x, glm::vec3( 1.0f, 0.0f, 0.0f ) );
    r = glm::rotate( r, rotation.y, glm::vec3( 0.0f, 1.0f, 0.0f ) );
    r = glm::rotate( r, rotation.z, glm::vec3( 0.0f, 0.0f, 1.0f ) );
}

using point_predicate_t = bool ( * )( float );

template < size_t N, point_predicate_t predicate >
static INLINE bool G_PointPlaneTest( const std::array< glm::vec3, N >& points, const plane_t& pln )
{
    for ( const glm::vec3& p: points )
    {
        float x = glm::dot( p, pln.normal ) - pln.d;

        if ( ( *predicate )( x ) )
        {
            return true;
        }
    }

    return false;
}

static INLINE float G_TripleProduct( const glm::vec3& a, const glm::vec3& b, const glm::vec3& c )
{
    return glm::dot( a, glm::cross( b, c ) );
}

//-------------------------------------------------------------------------------------------------------
// transform
//-------------------------------------------------------------------------------------------------------

INLINE transform_t::transform_t( void )
    : scale( 0.0f ), rotation( 0.0f ),
      translation( 0.0f ),
      top( 1.0f )
{
}

INLINE void transform_t::PushTransform( void )
{
    matStack.push( top );
}

INLINE void transform_t::PopTransform( void )
{
    matStack.pop();
}

INLINE const glm::mat4& transform_t::PeekTransform( void ) const
{
    return top;
}

INLINE void transform_t::SetScale( const glm::vec3& s )
{
    scale = s;
}

INLINE void transform_t::SetRotation( const glm::vec3& r )
{
    rotation = r;
}

INLINE void transform_t::SetTranslation( const glm::vec3& t )
{
    translation = t;
}

INLINE void transform_t::ApplyScale( void )
{
    top *= glm::scale( glm::mat4( 1.0f ), scale );
}

INLINE void transform_t::ApplyRotation( void )
{
    glm::mat4 r( 1.0f );
    G_RotateMatrixXYZ( r, rotation );
    top *= r;
}

INLINE void transform_t::ApplyTranslation( void )
{
    top *= glm::translate( glm::mat4( 1.0f ), translation );
}

INLINE void transform_t::ApplyScale( const glm::vec3& s )
{
    scale = s;
    ApplyScale();
}

INLINE void transform_t::ApplyRotation( const glm::vec3& r )
{
    rotation = r;
    ApplyRotation();
}

INLINE void transform_t::ApplyTranslation( const glm::vec3& t )
{
    translation = t;
    ApplyTranslation();
}

INLINE glm::mat3 transform_t::GetScale( void ) const
{
    return std::move( glm::mat3( glm::scale( glm::mat4( 1.0f ), scale ) ) );
}

INLINE glm::mat3 transform_t::GetRotation3( void ) const
{
    glm::mat4 r;
    G_RotateMatrixXYZ( r, rotation );
    return std::move( glm::mat3( r ) );
}

INLINE glm::mat4 transform_t::GetTranslation( void ) const
{
    return std::move( glm::translate( glm::mat4( 1.0f ), translation ) );
}

//-------------------------------------------------------------------------------------------------------
// bounding_box_t
//-------------------------------------------------------------------------------------------------------

INLINE glm::vec3 bounding_box_t::GetCornerIdentity( corner_t index ) const
{
    return glm::vec3(
        ( ( int32_t ) index & 1 ) ? 1.0f : -1.0f,
        ( ( int32_t ) index & 2 ) ? 1.0f : -1.0f,
        ( ( int32_t ) index & 4 ) ? 1.0f : -1.0f
    );
}

INLINE bool	bounding_box_t::InXRange( const glm::vec3& v ) const
{

    return v.x <= GetCorner( CORNER_MAX ).x && v.x >= GetCorner( CORNER_MIN ).x;
}

INLINE bool bounding_box_t::InYRange( const glm::vec3& v ) const
{

    return v.y <= GetCorner( CORNER_MAX ).y && v.y >= GetCorner( CORNER_MIN ).y;
}

INLINE bool bounding_box_t::InZRange( const glm::vec3& v ) const
{
#ifdef AABB_MAX_Z_LESS_THAN_MIN_Z
    return v.z >= GetCorner( CORNER_MAX ).z && v.z <= GetCorner( CORNER_MIN ).z;
#else
    return v.z <= GetCorner( CORNER_MAX ).z && v.z >= GetCorner( CORNER_MIN ).z;
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

INLINE const glm::mat4& bounding_box_t::GetTransform( void ) const
{
    return transform;
}

INLINE void bounding_box_t::SetCenter( const glm::vec3& position )
{
    transform[ 3 ] = glm::vec4( position, 1.0f );
}

INLINE void bounding_box_t::SetOrientation( const glm::mat3& orient )
{
    transform[ 0 ] = glm::vec4( orient[ 0 ], 0.0f );
    transform[ 1 ] = glm::vec4( orient[ 1 ], 0.0f );
    transform[ 2 ] = glm::vec4( orient[ 2 ], 0.0f );
}

INLINE const glm::vec4& bounding_box_t::operator[]( uint32_t i ) const
{
    assert( i < 4 );

    return transform[ i ];
}

//-------------------------------------------------------------------------------------------------------
// bounds_primitive_t
//-------------------------------------------------------------------------------------------------------

INLINE primitive_lookup_t* bounds_primitive_t::ToLookup( void )
{
    assert( type == BOUNDS_PRIM_LOOKUP );
    return ( primitive_lookup_t* ) this;
}

INLINE const primitive_lookup_t* bounds_primitive_t::ToLookup( void ) const
{
    assert( type == BOUNDS_PRIM_LOOKUP );
    return ( const primitive_lookup_t* ) this;
}


INLINE bounding_box_t* bounds_primitive_t::ToBox( void )
{
    assert( type == BOUNDS_PRIM_BOX );
    return ( bounding_box_t* ) this;
}

INLINE const bounding_box_t* bounds_primitive_t::ToBox( void ) const
{
    assert( type == BOUNDS_PRIM_BOX );
    return ( const bounding_box_t* ) this;
}

INLINE half_space_t* bounds_primitive_t::ToHalfSpace( void )
{
    assert( type == BOUNDS_PRIM_BOX );
    return ( half_space_t* ) this;
}
