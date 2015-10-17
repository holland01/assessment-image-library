#pragma once

#include <SDL2/SDL.h>
#include <stdint.h>
#include <array>
#include <glm/gtc/matrix_transform.hpp>
#include "entity.h"
#include "view.h"

// Simple API for first-person camera, using bullet physics bodies for physics/collision/etc.

enum class input_key : uint32_t
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
    V = 86
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

struct input_client : public entity
{
private:
    physics_body* get_body( void ) { return mMode == MODE_SPEC? mKinematicEnt.get(): mPhysEnt.get(); }

public:

    view_data mViewParams;

    std::array< uint8_t, 8 > mKeysPressed;

    enum client_mode
	{
		MODE_PLAY = 0,
		MODE_SPEC
	};

    client_mode mMode;

    input_client( void );
    input_client( const view_data& mViewParams );
    input_client( float width, float height, const glm::mat4& viewTransform, const glm::mat4& projection );

    bool    eval_key_press( input_key key ); // returns true if a camera key was pressed

    bool    eval_key_release( input_key key ); // same

    void    eval_mouse_move( float x, float y, bool calcRelative );

    void	sync( void ) override;

    void    set_physics( float mass, const glm::mat4& orientAndTranslate );

    void	add_dir( const glm::vec3& dir, float scale );

    void    perspective( float fovy, float width, float height, float znear, float zfar );

    void	clip_transform( const glm::mat4& proj );

    void	view_transform( const glm::mat4& mViewParams );

    void    update_view_data( void );

    void	position( const glm::vec3& origin );

    const glm::vec3& position( void ) const;

    glm::vec3   calc_direction( const glm::vec3& d ) const;

    glm::vec3   forward( void ) const { return std::move( calc_direction( G_DIR_FORWARD ) ); }

    glm::vec3   up( void ) const { return std::move( calc_direction( G_DIR_UP ) ); }

    glm::vec3   right( void ) const { return std::move( calc_direction( G_DIR_RIGHT ) ); }

    glm::vec3   world_direction( const glm::vec3& d ) const;

    const view_data& view_params( void ) const;

    void print_origin( void ) const;
};

INLINE glm::vec3 input_client::calc_direction( const glm::vec3& d ) const
{
    // If we have a body defined, then we're going to update its orientation
    // with the inverse orientation transform computed from the camera already;
    // this avoids a double transformation which screws things up.
    return world_direction( d );
}

INLINE glm::vec3 input_client::world_direction( const glm::vec3& v ) const
{
    glm::vec3 u( mViewParams.mInverseOrient * glm::vec4( v, 1.0f ) );
    return  glm::normalize( u );
}

INLINE void input_client::perspective( float fovy, float width, float height, float zNear, float zFar )
{
	fovy = glm::radians( fovy );

	float aspect = width / height;

    mViewParams.mClipTransform = glm::perspective( fovy, aspect, zNear, zFar );

	// Cache params for frustum culling
    mViewParams.mFovy = fovy;
    mViewParams.mAspect = aspect;
    mViewParams.mZNear = zNear;
    mViewParams.mZFar = zFar;
    mViewParams.mWidth = width;
    mViewParams.mHeight = height;
}

INLINE void input_client::clip_transform( const glm::mat4& proj )
{
    mViewParams.mClipTransform = proj;
}

INLINE void input_client::view_transform( const glm::mat4& view )
{
    mViewParams.mTransform = view;
}

INLINE void input_client::position( const glm::vec3& origin )
{
    mViewParams.mOrigin = origin;
}

INLINE const glm::vec3& input_client::position( void ) const
{
    return mViewParams.mOrigin;
}

INLINE const view_data& input_client::view_params( void ) const
{
    return mViewParams;
}
