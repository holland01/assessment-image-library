#pragma once

#include "def.h"
#include "input.h"
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

    return glm::vec3( forward );
}

INLINE glm::vec3 camera_t::Right( void ) const
{
    glm::vec4 right = viewParams.inverseOrient * glm::vec4( 1.0f, 0.0f, 0.0f, 1.0f );

    return glm::vec3( right );
}

INLINE glm::vec3 camera_t::Up( void ) const
{
    glm::vec4 up = viewParams.inverseOrient * glm::vec4( 0.0f, 1.0f, 0.0f, 1.0f );

    return glm::vec3( up );
}

INLINE void camera_t::Walk( float amount )
{
    viewParams.forward = Forward() * amount;
    viewParams.origin += viewParams.forward;
}

INLINE void camera_t::Strafe( float amount )
{
    viewParams.right = Right() * amount;
    viewParams.origin += viewParams.right;
}

INLINE void camera_t::Raise( float amount )
{
    viewParams.right = Up() * amount;
    viewParams.origin += viewParams.right;
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

} // namespace view
