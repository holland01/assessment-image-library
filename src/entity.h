#pragma once

#include "collision.h"
#include "geom.h"
#include <memory>
#include <glm/vec4.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct body_t;
struct pipeline_t;
struct view_params_t;

struct entity_t
{
    enum dependent_t
    {
        BOUNDS_DEPENDENT,
        BODY_DEPENDENT
    };

    dependent_t depType;

    glm::vec4 color;

    float size;

    std::shared_ptr< bounds_primitive_t > bounds;

    std::shared_ptr< body_t > body;

    entity_t( dependent_t dep,
              bounds_primitive_t* bounds = nullptr,
              body_t* body = nullptr,
              const glm::vec4& color = glm::vec4( 1.0f ) );

    virtual void Sync( void );

    glm::mat4 GenScaleTransform( void ) const;

    bounding_box_t* GetBoundsAsBox( void );

    const bounding_box_t* GetBoundsAsBox( void ) const;

    primitive_lookup_t* GetBoundsAsLookup( void );

    const primitive_lookup_t* GetBoundsAsLookup( void ) const;
};

INLINE glm::mat4 entity_t::GenScaleTransform( void ) const
{
    return glm::scale( glm::mat4( 1.0f ), glm::vec3( size ) );
}

INLINE bounding_box_t* entity_t::GetBoundsAsBox( void )
{
    assert( bounds->type == BOUNDS_PRIM_BOX );
    return ( bounding_box_t* )bounds.get();
}

INLINE const bounding_box_t* entity_t::GetBoundsAsBox( void ) const
{
    assert( bounds->type == BOUNDS_PRIM_BOX );
    return ( const bounding_box_t* )bounds.get();
}

INLINE primitive_lookup_t* entity_t::GetBoundsAsLookup( void )
{
    assert( bounds->type == BOUNDS_PRIM_LOOKUP );
    return ( primitive_lookup_t* )bounds.get();
}

INLINE const primitive_lookup_t* entity_t::GetBoundsAsLookup( void ) const
{
    assert( bounds->type == BOUNDS_PRIM_LOOKUP );
    return ( const primitive_lookup_t* )bounds.get();
}






