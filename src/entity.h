#pragma once

#include "geom/geom.h"
#include <memory>
#include <glm/vec4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "physics_entity.h"

struct render_pipeline;
struct view_data;

//-------------------------------------------------------------------------------------------------------
// entity_bounds_primitive_t
//-------------------------------------------------------------------------------------------------------

#define ENTITY_GET_BOX( e, useType ) ( ( e ).query_bounds( useType )->to_box() )
#define ENTITY_PTR_GET_BOX( e, useType ) ( ( e )->query_bounds( useType )->to_box() )

#define ENTITY_PTR_GET_LOOKUP( e, useType ) ( ( e )->query_bounds( useType )->to_lookup() )

#define ENTITY_PTR_GET_HALFSPACE( e, useType ) ( ( e )->query_bounds( useType )->to_halfspace() )

enum entity_bounds_use_flags
{
    ENTITY_BOUNDS_AIR_COLLIDE = 0x1,
    ENTITY_BOUNDS_MOVE_COLLIDE = 0x2,
    ENTITY_BOUNDS_AREA_EVAL = 0x4,
    ENTITY_BOUNDS_ALL = 0x7
};

enum entity_sync_flags
{
    ENTITY_SYNC_APPLY_SCALE = 0x1
};

struct entity_bounds_primitive
{
    uint32_t usageFlags;

    std::unique_ptr< bounds_primitive > bounds;

    std::unique_ptr< entity_bounds_primitive > next;

    entity_bounds_primitive( uint32_t usageFlags = ENTITY_BOUNDS_ALL, bounds_primitive* bounds = nullptr );
};

//-------------------------------------------------------------------------------------------------------
// entity_t
//-------------------------------------------------------------------------------------------------------

struct physics_world;

struct entity
{
protected:
    friend struct map_tile;
    friend struct map_tile_generator;

    std::unique_ptr< entity_bounds_primitive > mBounds;

    std::unique_ptr< physics_entity > mPhysEnt;

public:

    glm::vec4 mColor;

    glm::vec3 mSize;

    entity( const glm::vec4& color = glm::vec4( 1.0f ) );

    void add_bounds( uint32_t usageFlags, bounds_primitive* mBounds );

    bounds_primitive* query_bounds( uint32_t flags );

    const bounds_primitive* query_bounds( uint32_t flags ) const;

    void orient_to( const glm::vec3& v );

    void add_to_world( physics_world& world );

    void remove_from_world( physics_world& world );

    glm::mat4 scale_transform( void ) const { return glm::scale( glm::mat4( 1.0f ), mSize ); }

    const physics_entity& physics_data( void ) const { return *mPhysEnt; }

    virtual void sync( void );
};







