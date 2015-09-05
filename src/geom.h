#pragma once

#include "def.h"
#include "renderer.h"
#include "glm_ext.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <stdint.h>
#include <stdio.h>
#include <memory>
#include <stack>
#include <unordered_set>

struct plane;
struct ray;

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
static INLINE glm::mat3 orient_by_direction( const glm::vec3& dir, const glm::vec3& rightAxis, const glm::vec3& backAxis );

static INLINE void rotate_matrix_xyz( glm::mat4& r, const glm::vec3& rotation );

using point_predicate_t = bool ( * )( float );
template < size_t N, point_predicate_t predicate >
static INLINE bool test_point_plane( const std::array< glm::vec3, N >& points, const plane& plane );

static INLINE float triple_product( const glm::vec3& a, const glm::vec3& b, const glm::vec3& c );

glm::vec3 plane_project( const glm::vec3& origin, const plane& p );

bool test_ray_ray( const ray& r0, const ray& r1, float& t0, float& t1 );


//-------------------------------------------------------------------------------------------------------
// plane_t
//-------------------------------------------------------------------------------------------------------

struct plane
{
    float       d = 0.0f;
    glm::vec3   normal = glm::vec3( 0.0f );
    glm::vec3   point = glm::vec3( 0.0f );
};

//-------------------------------------------------------------------------------------------------------
// ray_t
//-------------------------------------------------------------------------------------------------------

struct ray
{
	glm::vec3 p;
	glm::vec3 d;
	float t;

	ray( const glm::vec3& position = glm::vec3( 0.0f ),
		 const glm::vec3& dir = glm::vec3( 0.0f ),
		 float t_ = 1.0f )
        : p( position ),
		  d( dir ),
		  t( t_ )
    {}

	ray( ray&& ) = delete;
	ray& operator=( ray&& ) = delete;

	ray( const ray& r )
		: p( r.p ),
		  d( r.d ),
		  t( r.t )
	{
	}

	ray& operator=( const ray& r )
	{
		if ( this != &r )
		{
			p = r.p;
			d = r.d;
			t = r.t;
		}

		return *this;
	}

	glm::vec3 calc_position( void ) const
	{
		return p + d * t;
	}
};

//-------------------------------------------------------------------------------------------------------
// transform_t
//-------------------------------------------------------------------------------------------------------

struct transform_stack
{
protected:
    glm::vec3 mScale;

    glm::vec3 mRotation;

    glm::vec3 mTranslation;

    glm::mat4 mTop;

    std::stack< glm::mat4 > mMatStack;

public:
    transform_stack( void );

    void push( void );

    void pop( void );

    const glm::mat4& peek( void ) const;

    void scale( const glm::vec3& s );

    void rotation( const glm::vec3& r );

    void translation( const glm::vec3& t );

    void apply_scale( void );

    void apply_rotation( void );

    void apply_translation( void );

    void apply_scale( const glm::vec3& s );

    void apply_rotation( const glm::vec3& r );

    void apply_translation( const glm::vec3& t );

    glm::mat3 scale( void ) const;

    glm::mat3 rotation( void ) const;

    glm::mat4 translation( void ) const;
};

//-------------------------------------------------------------------------------------------------------
// bounds_primitive_t
//-------------------------------------------------------------------------------------------------------

enum bounds_primtype
{   BOUNDS_PRIM_HALFSPACE = 0,
    BOUNDS_PRIM_BOX,
    BOUNDS_PRIM_LOOKUP,
    NUM_BOUNDS_PRIMTYPE
};


struct primitive_lookup;
struct obb;
struct halfspace;

struct bounds_primitive
{
protected:
    bounds_primitive( bounds_primtype type_ )
        : type( type_ )
    {}

    bounds_primitive( const bounds_primitive& c )
        : type( c.type )
    {
    }

    bounds_primitive( const bounds_primitive&& m )
        : type( m.type )
    {
    }

public:
    const bounds_primtype type;

    primitive_lookup*         to_lookup( void );

    const primitive_lookup*   to_lookup( void ) const;

    obb*             to_box( void );

    const obb*       to_box( void ) const;

    halfspace*               to_halfspace( void );
};

//-------------------------------------------------------------------------------------------------------
// primitive_lookup_t
//-------------------------------------------------------------------------------------------------------

struct primitive_lookup : public bounds_primitive
{
    static const int32_t LOOKUP_UNSET = -1;

    bounds_primtype lookupType;
    int32_t index = 0;

    primitive_lookup( bounds_primtype lookupType_, int32_t index_ = LOOKUP_UNSET )
        : bounds_primitive( BOUNDS_PRIM_LOOKUP ),
          lookupType( lookupType_ ),
          index( index_ )
    {}
};

//-------------------------------------------------------------------------------------------------------
// half_space_t
//-------------------------------------------------------------------------------------------------------

struct obb;

struct halfspace : public bounds_primitive
{
	glm::mat3 extents;
	glm::vec3 origin;
	float distance;

    halfspace( void );
    halfspace( const glm::mat3& extents, const glm::vec3& origin, float distance );
    halfspace( const obb& bounds, const glm::vec3& normal );

    halfspace( const halfspace& c );
    halfspace& operator=( halfspace c );

    bool test_bounds( glm::vec3& normal, const glm::mat3& extents, const glm::vec3& origin ) const;
    void draw( imm_draw& drawer ) const;
};

//-------------------------------------------------------------------------------------------------------
// obb
//-------------------------------------------------------------------------------------------------------

struct obb : public bounds_primitive
{
private:
    glm::mat4 mAxes;

public:
    glm::vec4 mColor;

    using pointset3D_t = std::unordered_set< glm::vec3 >;

    using pointlist3D_t = std::array< glm::vec3, 8 >;

    using maxmin_pair3D_t = glm::ext::vec3_maxmin_pair_t;

    enum face_type
    {
        FACE_TOP = 0,
        FACE_RIGHT,
        FACE_FRONT,
        FACE_LEFT,
        FACE_BACK,
        FACE_BOTTOM
    };

    enum corner_type
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

    obb( const glm::mat4& mAxes = glm::mat4( 1.0f ) );

    obb( obb&& m );

    obb( const obb& c );

    obb& operator =( obb c ) = delete;

    bool			encloses( const obb& box ) const;

    glm::vec3       center( void ) const;

    glm::vec3       size( void ) const;

    glm::vec3       radius( void ) const;

    glm::vec3       corner( corner_type index ) const;

    glm::vec3       corner_identity( corner_type index ) const;

    const glm::mat4& axes( void ) const;

    void            axes( const glm::mat4& m ) { mAxes = m; }

    glm::mat3       linear_axes( void ) const { return std::move( glm::mat3( axes() ) ); }

    glm::mat3       inv_linear_axes( void ) const { return std::move( glm::mat3( glm::inverse( axes() ) ) ); }

    void			edges_from_corner( corner_type index, glm::mat3& edges ) const;

    void            points( pointlist3D_t& points ) const;

    pointset3D_t    face_project( const plane& facePlane, const pointlist3D_t& sourcePoints ) const;

    void			face_plane( face_type face, plane& plane_t ) const;

    void			color( const glm::vec4& mColor );

    void            center( const glm::vec3& position );

    void            orientation( const glm::mat3& orient );

    const glm::vec4& operator[]( uint32_t i ) const;

    bool			range_x( const glm::vec3& v ) const;

    bool			range_y( const glm::vec3& v ) const;

    bool			range_z( const glm::vec3& v ) const;

    // pass isTransformed = "true" if v has already been transformed relative to the linear inverse of this bounds
    bool			range( glm::vec3 v, bool isTransformed ) const;

    bool			intersects( glm::vec3& normal, const obb& bounds ) const;

    bool			intersects( glm::vec3& normal, const halfspace& halfSpace ) const;

	bool            ray_intersection( ray& r, bool earlyOut = true ) const;

    maxmin_pair3D_t maxmin( bool inverse ) const;
};

#include "geom.inl"


//-------------------------------------------------------------------------------------------------------
// util
//-------------------------------------------------------------------------------------------------------

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

static INLINE void rotate_matrix_xyz( glm::mat4& r, const glm::vec3& rotation )
{
    r = glm::rotate( glm::mat4( 1.0f ), rotation.x, glm::vec3( 1.0f, 0.0f, 0.0f ) );
    r = glm::rotate( r, rotation.y, glm::vec3( 0.0f, 1.0f, 0.0f ) );
    r = glm::rotate( r, rotation.z, glm::vec3( 0.0f, 0.0f, 1.0f ) );
}

using point_predicate_t = bool ( * )( float );

template < size_t N, point_predicate_t predicate >
static INLINE bool test_point_plane( const std::array< glm::vec3, N >& points, const plane& pln )
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

static INLINE float triple_product( const glm::vec3& a, const glm::vec3& b, const glm::vec3& c )
{
    return glm::dot( a, glm::cross( b, c ) );
}

//-------------------------------------------------------------------------------------------------------
// transform
//-------------------------------------------------------------------------------------------------------

INLINE transform_stack::transform_stack( void )
    : mScale( 0.0f ), mRotation( 0.0f ),
      mTranslation( 0.0f ),
      mTop( 1.0f )
{
}

INLINE void transform_stack::push( void )
{
    mMatStack.push( mTop );
}

INLINE void transform_stack::pop( void )
{
    mMatStack.pop();
}

INLINE const glm::mat4& transform_stack::peek( void ) const
{
    return mTop;
}

INLINE void transform_stack::scale( const glm::vec3& s )
{
    mScale = s;
}

INLINE void transform_stack::rotation( const glm::vec3& r )
{
    mRotation = r;
}

INLINE void transform_stack::translation( const glm::vec3& t )
{
    mTranslation = t;
}

INLINE void transform_stack::apply_scale( void )
{
    mTop *= glm::scale( glm::mat4( 1.0f ), mScale );
}

INLINE void transform_stack::apply_rotation( void )
{
    glm::mat4 r( 1.0f );
    rotate_matrix_xyz( r, mRotation );
    mTop *= r;
}

INLINE void transform_stack::apply_translation( void )
{
    mTop *= glm::translate( glm::mat4( 1.0f ), mTranslation );
}

INLINE void transform_stack::apply_scale( const glm::vec3& s )
{
    mScale = s;
    apply_scale();
}

INLINE void transform_stack::apply_rotation( const glm::vec3& r )
{
    mRotation = r;
    apply_rotation();
}

INLINE void transform_stack::apply_translation( const glm::vec3& t )
{
    mTranslation = t;
    apply_translation();
}

INLINE glm::mat3 transform_stack::scale( void ) const
{
    return std::move( glm::mat3( glm::scale( glm::mat4( 1.0f ), mScale ) ) );
}

INLINE glm::mat3 transform_stack::rotation( void ) const
{
    glm::mat4 r;
    rotate_matrix_xyz( r, mRotation );
    return std::move( glm::mat3( r ) );
}

INLINE glm::mat4 transform_stack::translation( void ) const
{
    return std::move( glm::translate( glm::mat4( 1.0f ), mTranslation ) );
}

//-------------------------------------------------------------------------------------------------------
// bounding_box_t
//-------------------------------------------------------------------------------------------------------

INLINE glm::vec3 obb::corner_identity( corner_type index ) const
{
    return glm::vec3(
        ( ( int32_t ) index & 1 ) ? 1.0f : -1.0f,
        ( ( int32_t ) index & 2 ) ? 1.0f : -1.0f,
        ( ( int32_t ) index & 4 ) ? 1.0f : -1.0f
    );
}

INLINE bool	obb::range_x( const glm::vec3& v ) const
{
	glm::vec3 max( corner( CORNER_MAX ) );
	glm::vec3 min( corner( CORNER_MIN ) );

	return v.x <= max.x && v.x >= min.x;
}

INLINE bool obb::range_y( const glm::vec3& v ) const
{
	glm::vec3 max( corner( CORNER_MAX ) );
	glm::vec3 min( corner( CORNER_MIN ) );

	return v.y <= max.y && v.y >= min.y;
}

INLINE bool obb::range_z( const glm::vec3& v ) const
{
#ifdef AABB_MAX_Z_LESS_THAN_MIN_Z
    return v.z >= GetCorner( CORNER_MAX ).z && v.z <= GetCorner( CORNER_MIN ).z;
#else
	glm::vec3 max( corner( CORNER_MAX ) );
	glm::vec3 min( corner( CORNER_MIN ) );

	return v.z <= max.z && v.z >= min.z;
#endif
}

INLINE void obb::points( pointlist3D_t& points ) const
{
    points[ 0 ] = corner( ( corner_type ) 0 );
    points[ 1 ] = corner( ( corner_type ) 1 );
    points[ 2 ] = corner( ( corner_type ) 2 );
    points[ 3 ] = corner( ( corner_type ) 3 );
    points[ 4 ] = corner( ( corner_type ) 4 );
    points[ 5 ] = corner( ( corner_type ) 5 );
    points[ 6 ] = corner( ( corner_type ) 6 );
    points[ 7 ] = corner( ( corner_type ) 7 );
}

INLINE const glm::mat4& obb::axes( void ) const
{
    return mAxes;
}

INLINE void obb::center( const glm::vec3& position )
{
    mAxes[ 3 ] = glm::vec4( position, 1.0f );
}

INLINE void obb::orientation( const glm::mat3& orient )
{
    mAxes[ 0 ] = glm::vec4( orient[ 0 ], 0.0f );
    mAxes[ 1 ] = glm::vec4( orient[ 1 ], 0.0f );
    mAxes[ 2 ] = glm::vec4( orient[ 2 ], 0.0f );
}

INLINE const glm::vec4& obb::operator[]( uint32_t i ) const
{
    assert( i < 4 );

    return mAxes[ i ];
}

//-------------------------------------------------------------------------------------------------------
// bounds_primitive_t
//-------------------------------------------------------------------------------------------------------

INLINE primitive_lookup* bounds_primitive::to_lookup( void )
{
    assert( type == BOUNDS_PRIM_LOOKUP );
    return ( primitive_lookup* ) this;
}

INLINE const primitive_lookup* bounds_primitive::to_lookup( void ) const
{
    assert( type == BOUNDS_PRIM_LOOKUP );
    return ( const primitive_lookup* ) this;
}


INLINE obb* bounds_primitive::to_box( void )
{
    assert( type == BOUNDS_PRIM_BOX );
    return ( obb* ) this;
}

INLINE const obb* bounds_primitive::to_box( void ) const
{
    assert( type == BOUNDS_PRIM_BOX );
    return ( const obb* ) this;
}

INLINE halfspace* bounds_primitive::to_halfspace( void )
{
    assert( type == BOUNDS_PRIM_BOX );
    return ( halfspace* ) this;
}

