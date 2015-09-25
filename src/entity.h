#pragma once

#include "geom/geom.h"
#include <memory>
#include <glm/vec4.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct rigid_body;
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

struct entity
{
private:
    friend struct map_tile;
    friend struct map_tile_generator;

    std::unique_ptr< entity_bounds_primitive > mBounds;

	uint32_t mSyncOpt;

public:
    enum dependent_t
    {
        BOUNDS_DEPENDENT,
        BODY_DEPENDENT
    };

    dependent_t mDepType;

    glm::vec4 mColor;

    glm::vec3 mSize;

    std::shared_ptr< rigid_body > mBody;

	entity(  dependent_t dep,
			 rigid_body* mBody = nullptr,
			 const glm::vec4& mColor = glm::vec4( 1.0f ) );

    virtual void sync( void );

    glm::mat4 scale_transform( void ) const { return glm::scale( glm::mat4( 1.0f ), mSize ); }

    void add_bounds( uint32_t usageFlags, bounds_primitive* mBounds );

    bounds_primitive* query_bounds( uint32_t flags );

    const bounds_primitive* query_bounds( uint32_t flags ) const;

    void orient_to( const glm::vec3& v );

	void sync_options( uint32_t opt ) { mSyncOpt = opt; }

	uint32_t sync_options( void ) const { return mSyncOpt; }
};







