#pragma once

#include "def.h"
#include "input.h"
#include "geom.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <array>

namespace view {

struct params_t
{
    glm::vec3   forward;
    glm::vec3   up;
    glm::vec3   right;

    glm::vec3   origin;

    float       fovy, aspect, zNear, zFar;
    float		width, height;

    glm::mat4   transform;

    glm::mat4   orientation;
    glm::mat4   inverseOrient;

    glm::mat4   clipTransform;

    params_t( void );
};

//-------------------------------------------------------------------------------------------------------
// Camera
//-------------------------------------------------------------------------------------------------------

class camera_t
{
    params_t    viewParams;

    glm::vec3   currRot, lastRot;

    glm::vec3   lastMouse;

    std::array< uint8_t, 8 > keysPressed;

public:

    camera_t( void );

    camera_t( const params_t& viewParams, const glm::vec3& currRot );

    camera_t( float width, float height, const glm::mat4& viewParams, const glm::mat4& projection );

    float	moveStep;

    void    EvalKeyPress( input_key_t key );
    void    EvalKeyRelease( input_key_t key );
    void    EvalMouseMove( float x, float y, bool calcRelative );

    void    Update( void );

    void    Walk( float amount );
    void    Strafe( float amount );
    void    Raise( float amount );

    void    SetPerspective( float fovy, float width, float height, float znear, float zfar );
    void	SetClipTransform( const glm::mat4& proj );
    void	SetViewTransform( const glm::mat4& viewParams );

    void	SetViewOrigin( const glm::vec3& origin );

    glm::vec3   Forward( void ) const;
    glm::vec3   Up( void ) const;
    glm::vec3   Right( void ) const;

    const params_t& GetViewParams( void ) const;

    friend class Test;
};

INLINE glm::vec3 camera_t::Forward( void ) const
{
	glm::vec4 forward = viewParams.inverseOrient * glm::vec4( 0.0f, 0.0f, -1.0f, 1.0f );

	return glm::normalize( glm::vec3( forward ) );
}

INLINE glm::vec3 camera_t::Right( void ) const
{
    glm::vec4 right = viewParams.inverseOrient * glm::vec4( 1.0f, 0.0f, 0.0f, 1.0f );

	return glm::normalize( glm::vec3( right ) );
}

INLINE glm::vec3 camera_t::Up( void ) const
{
    glm::vec4 up = viewParams.inverseOrient * glm::vec4( 0.0f, 1.0f, 0.0f, 1.0f );

	return glm::normalize( glm::vec3( up ) );
}

INLINE void camera_t::Walk( float amount )
{
	//viewParams.forward = Forward() * amount;
	viewParams.origin += viewParams.forward * amount;
}

INLINE void camera_t::Strafe( float amount )
{
	///viewParams.right = Right() * amount;
	viewParams.origin += viewParams.right * amount;
}

INLINE void camera_t::Raise( float amount )
{
	//viewParams.right = Up() * amount;
	viewParams.origin += viewParams.up * amount;
}

INLINE void camera_t::SetPerspective( float fovy, float width, float height, float zNear, float zFar )
{
    fovy = glm::radians( fovy );

    float aspect = width / height;

    viewParams.clipTransform = glm::perspective( fovy, aspect, zNear, zFar );

    // Cache params for frustum culling
    viewParams.fovy = fovy;
    viewParams.aspect = aspect;
    viewParams.zNear = zNear;
    viewParams.zFar = zFar;
    viewParams.width = width;
    viewParams.height = height;
}

INLINE void camera_t::SetClipTransform( const glm::mat4& proj )
{
    viewParams.clipTransform = proj;
}

INLINE void camera_t::SetViewTransform( const glm::mat4& view )
{
    viewParams.transform = view;
}

INLINE void camera_t::SetViewOrigin( const glm::vec3& origin )
{
    viewParams.origin = origin;
}

INLINE const params_t& camera_t::GetViewParams( void ) const
{
    return viewParams;
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
	geom::plane_t    frustPlanes[ FRUST_NUM_PLANES ];

	mutable uint32_t acceptCount;

	mutable uint32_t rejectCount;

	glm::mat4 mvp;

	glm::vec4 CalcPlaneFromOrigin( const glm::vec4& position, const glm::vec4& origin );

public:

	frustum_t( void );

	~frustum_t( void );

	void    Update( const params_t& params );

	void	PrintMetrics( void ) const;

	void	ResetMetrics( void ) const { rejectCount = 0; acceptCount = 0; }

	bool    IntersectsBox( const geom::bounding_box_t& box ) const;
};

INLINE void frustum_t::PrintMetrics( void ) const
{
	printf( "Reject Count: %iu; Accept Count: %iu\r", rejectCount, acceptCount );
}

} // namespace view
