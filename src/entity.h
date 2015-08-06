#pragma once

#include "def.h"
#include <memory>
#include <glm/vec4.hpp>

struct bounding_box_t;
struct body_t;

class entity_t
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

    glm::vec3 orientation, scale, translation;

public:

    std::shared_ptr< bounding_box_t > bounds;
    std::shared_ptr< body_t > body;

    entity_t( dependent_t dep, bounding_box_t* bounds = nullptr, body_t* body = nullptr, const glm::vec4& color = glm::vec4( 1.0f ) );

    virtual void Sync( void );

    const glm::vec4& GetColor( void ) const;

    void SetColor( const glm::vec3& color, float alpha = 1.0f );
};

INLINE const glm::vec4& entity_t::GetColor( void ) const
{
    return color;
}

INLINE void entity_t::SetColor( const glm::vec3& color, float alpha )
{
    this->color = std::move( glm::vec4( color, alpha ) );
}




