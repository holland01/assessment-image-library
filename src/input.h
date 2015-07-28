#pragma once

#include <SDL2/SDL.h>
#include <stdint.h>
#include <array>
#include <glm/gtc/matrix_transform.hpp>
#include "physics.h"
#include "view.h"

enum class input_key_t : uint32_t
{
#ifdef EMSCRIPTEN
    ESC = 27,
    W = 87,
    S = 83,
    A = 65,
    D = 68,
    SPACE = 32,
    LSHIFT = 16,
    E = 69,
	Q = 81,
	R = 82,
	V
#else
    ESC = SDLK_ESCAPE,
    W = SDLK_w,
    S = SDLK_s,
    A = SDLK_a,
    D = SDLK_d,
    SPACE = SDLK_SPACE,
    LSHIFT = SDLK_LSHIFT,
    E = SDLK_e,
	Q = SDLK_q,
	R = SDLK_r
#endif
};

struct input_client_t
{
	view::params_t viewParams;
	phys::body_t body;
	geom::bounding_box_t bounds;

	std::array< uint8_t, 8 > keysPressed;

	input_client_t( void );

	input_client_t( const view::params_t& viewParams );

	input_client_t( float width, float height, const glm::mat4& viewTransform, const glm::mat4& projection );

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

	void	SetPosition( const glm::vec3& origin );

	glm::vec3   Forward( void ) const;
	glm::vec3   Up( void ) const;
	glm::vec3   Right( void ) const;

	const view::params_t& GetViewParams( void ) const;

	void PrintOrigin( void ) const;
};

INLINE glm::vec3 input_client_t::Forward( void ) const
{
	glm::vec4 forward = viewParams.inverseOrient * glm::vec4( 0.0f, 0.0f, -1.0f, 1.0f );

	return glm::normalize( glm::vec3( forward ) );
}

INLINE glm::vec3 input_client_t::Right( void ) const
{
	glm::vec4 right = viewParams.inverseOrient * glm::vec4( 1.0f, 0.0f, 0.0f, 1.0f );

	return glm::normalize( glm::vec3( right ) );
}

INLINE glm::vec3 input_client_t::Up( void ) const
{
	glm::vec4 up = viewParams.inverseOrient * glm::vec4( 0.0f, 1.0f, 0.0f, 1.0f );

	return glm::normalize( glm::vec3( up ) );
}

INLINE void input_client_t::Walk( float amount )
{
	viewParams.origin += viewParams.forward * amount;
	//body.forceAccum += viewParams.forward * amount;
}

INLINE void input_client_t::Strafe( float amount )
{
	viewParams.origin += viewParams.right * amount;
	body.forceAccum += viewParams.right * amount;
}

INLINE void input_client_t::Raise( float amount )
{
	viewParams.origin += viewParams.up * amount;
	//body.forceAccum += viewParams.up * amount;
}

INLINE void input_client_t::SetPerspective( float fovy, float width, float height, float zNear, float zFar )
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

INLINE void input_client_t::SetClipTransform( const glm::mat4& proj )
{
	viewParams.clipTransform = proj;
}

INLINE void input_client_t::SetViewTransform( const glm::mat4& view )
{
	viewParams.transform = view;
}

INLINE void input_client_t::SetPosition( const glm::vec3& origin )
{
	viewParams.origin = origin;
}

INLINE const view::params_t& input_client_t::GetViewParams( void ) const
{
	return viewParams;
}
