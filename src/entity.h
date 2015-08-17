#pragma once

#include "def.h"
#include <memory>
#include <glm/vec4.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct bounding_box_t;
struct body_t;

class entity
{
public:
    enum dependent_t
    {
        BOUNDS_DEPENDENT,
        BODY_DEPENDENT
    };

protected:
    dependent_t depType;
    glm::vec4 color;
    float size;

public:

    std::shared_ptr< bounding_box_t > bounds;
    std::shared_ptr< body_t > body;

    entity( dependent_t dep, bounding_box_t* bounds = nullptr, body_t* body = nullptr, const glm::vec4& color = glm::vec4( 1.0f ) );

    virtual void Sync( void );

    const glm::vec4& GetColor( void ) const;

    glm::mat4 GenScaleTransform( void ) const;

    void SetColor( const glm::vec3& color, float alpha = 1.0f );

    void SetSize( float sz );
};

INLINE const glm::vec4& entity::GetColor( void ) const
{
    return color;
}

INLINE glm::mat4 entity::GenScaleTransform( void ) const
{
    return glm::scale( glm::mat4( 1.0f ), glm::vec3( size ) );
}

INLINE void entity::SetColor( const glm::vec3& color, float alpha )
{
    this->color = std::move( glm::vec4( color, alpha ) );
}

INLINE void entity::SetSize( float sz )
{
    size = sz;
}





