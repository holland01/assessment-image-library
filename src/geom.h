#pragma once

#include "def.h"
#include "renderer.h"
#include "glm_ext.hpp"
#include "collision_contact.h"
#include "geom_plane.h"
#include "geom_transform_data.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <stdint.h>
#include <stdio.h>
#include <memory>
#include <stack>
#include <unordered_set>

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

bool test_ray_ray( const ray& r0, const ray& r1, float& t0, float& t1 );

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

    bool test_bounds( contact::list_t& contacts, const glm::mat3& extents, const glm::vec3& origin ) const;
    void draw( imm_draw& drawer ) const;
};

//-------------------------------------------------------------------------------------------------------
// obb
//-------------------------------------------------------------------------------------------------------

struct point_project_pair;

struct obb : public bounds_primitive
{
private:
	transform_data mT;

public:
    glm::vec4 mColor;

    using pointset3D_t = std::unordered_set< point_project_pair >;

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

	obb( glm::mat3 axes = glm::mat3( 1.0f ) );

    obb( obb&& m );

    obb( const obb& c );

    obb& operator =( obb c ) = delete;

    bool			encloses( const obb& box ) const;

	const glm::vec3& origin( void ) const { return mT.mOrigin; }

	const glm::vec3& extents( void ) const { return mT.mExtents; }

    glm::vec3       radius( void ) const;

    glm::vec3       corner( corner_type index ) const;

	glm::mat4		world_transform( void ) const { return std::move( mT.world_transform() ); }

	const glm::mat3& axes( void ) const { return mT.mAxes; }

	void            axes( const glm::mat3 m ) { mT.mAxes = std::move( m ); }

	glm::mat3		inv_axes( void ) const { return std::move( glm::inverse( axes() ) ); }

    void			edges_from_corner( corner_type index, glm::mat3& edges ) const;

	void            get_world_space_points( pointlist3D_t& get_world_space_points ) const;

    pointset3D_t    face_project( const plane& facePlane, const pointlist3D_t& sourcePoints ) const;

    void			face_plane( face_type face, plane& plane_t ) const;

    void			color( const glm::vec4& mColor );

	void            origin( const glm::vec3& position );

    // pass isTransformed = "true" if v has already been transformed relative to the linear inverse of this bounds
    bool			range( glm::vec3 v, bool isTransformed ) const;

    bool			intersects( contact::list_t& contacts, const obb& bounds ) const;

    bool			intersects( contact::list_t& contacts, const halfspace& halfSpace ) const;

	bool            ray_intersection( ray& r, bool earlyOut = true ) const;

    maxmin_pair3D_t maxmin( bool inverse ) const;
};

INLINE glm::vec3 obb::corner( corner_type index ) const
{
	return std::move( glm::vec3(
		( ( int32_t ) index & 1 ) ? 1.0f : -1.0f,
		( ( int32_t ) index & 2 ) ? 1.0f : -1.0f,
		( ( int32_t ) index & 4 ) ? 1.0f : -1.0f
	) );
}

INLINE void obb::get_world_space_points( pointlist3D_t& points ) const
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

INLINE void obb::origin( const glm::vec3& position )
{
	mT.mOrigin = position;
}

#include "geom.inl"
