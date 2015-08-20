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
// half_space_t
//-------------------------------------------------------------------------------------------------------

struct half_space_t
{
	glm::mat3 extents;
	glm::vec3 origin;
	float distance;

    half_space_t( void );
    half_space_t( const glm::mat3& extents, const glm::vec3& origin, float distance );

	bool TestBounds( glm::vec3& normal, const glm::mat3& extents, const glm::vec3& origin ) const;
	void Draw( imm_draw_t& drawer ) const;
};

//-------------------------------------------------------------------------------------------------------
// bounding_box_t
//-------------------------------------------------------------------------------------------------------

struct bounding_box_t
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

