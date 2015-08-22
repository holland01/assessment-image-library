#pragma once

#include "geom.h"
#include <memory>
#include <glm/vec4.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct body_t;
struct pipeline_t;
struct view_params_t;

//-------------------------------------------------------------------------------------------------------
// entity_bounds_primitive_t
//-------------------------------------------------------------------------------------------------------

#define ENTITY_GET_BOX( e, useType ) ( ( e ).QueryBounds( useType )->ToBox() )
#define ENTITY_PTR_GET_BOX( e, useType ) ( ( e )->QueryBounds( useType )->ToBox() )

#define ENTITY_PTR_GET_LOOKUP( e, useType ) ( ( e )->QueryBounds( useType )->ToLookup() )

#define ENTITY_PTR_GET_HALFSPACE( e, useType ) ( ( e )->QueryBounds( useType )->ToHalfSpace() )

enum entity_bounds_use_flags_t
{
    ENTITY_BOUNDS_AIR_COLLIDE = 0x1,
    ENTITY_BOUNDS_MOVE_COLLIDE = 0x2,
    ENTITY_BOUNDS_AREA_EVAL = 0x4,
    ENTITY_BOUNDS_ALL = 0x7
};

struct entity_bounds_primitive_t
{
    uint32_t usageFlags;

    std::unique_ptr< bounds_primitive_t > bounds;

    std::unique_ptr< entity_bounds_primitive_t > next;

    entity_bounds_primitive_t( uint32_t usageFlags = ENTITY_BOUNDS_ALL, bounds_primitive_t* bounds = nullptr );
};

//-------------------------------------------------------------------------------------------------------
// entity_t
//-------------------------------------------------------------------------------------------------------

struct entity_t
{
private:
    friend struct map_tile_t;
    friend struct tile_generator_t;

    std::unique_ptr< entity_bounds_primitive_t > bounds;

public:
    enum dependent_t
    {
        BOUNDS_DEPENDENT,
        BODY_DEPENDENT
    };

    dependent_t depType;

    glm::vec4 color;

    float size;

    std::shared_ptr< body_t > body;

    entity_t( dependent_t dep,
              body_t* body = nullptr,
              const glm::vec4& color = glm::vec4( 1.0f ) );

    virtual void Sync( void );

    glm::mat4 GenScaleTransform( void ) const;

    void AddBounds( uint32_t usageFlags, bounds_primitive_t* bounds );

    bounds_primitive_t* QueryBounds( uint32_t flags );

    const bounds_primitive_t* QueryBounds( uint32_t flags ) const;
};

INLINE glm::mat4 entity_t::GenScaleTransform( void ) const
{
    return glm::scale( glm::mat4( 1.0f ), glm::vec3( size ) );
}





