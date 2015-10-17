#pragma once

#include "geom/geom.h"
#include <memory>
#include <glm/vec4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "physics_body.h"

struct render_pipeline;
struct view_data;

//-------------------------------------------------------------------------------------------------------
// entity
//-------------------------------------------------------------------------------------------------------

struct physics_world;

enum class entity_move_state
{
    automatic = 0,
    manual = 1
};

struct entity
{
protected:
    friend struct map_tile;
    friend struct map_tile_generator;

    std::unique_ptr< physics_body > mPhysEnt, mKinematicEnt;

    entity_move_state mMoveState;

public:

    glm::vec4 mColor;

    glm::vec3 mSize;

    entity( const glm::vec4& color = glm::vec4( 1.0f ) );

    void toggle_kinematic( void );

    glm::mat4 scale_transform( void ) const { return glm::scale( glm::mat4( 1.0f ), mSize ); }

    const physics_body& normal_body( void ) const { assert( mPhysEnt ); return *mPhysEnt; }

    const physics_body& kinematic_body( void ) const { assert( mKinematicEnt ); return *mKinematicEnt; }

    void orient_kinematic_to( const glm::vec3& p );

    void set_normal_body_activation_state( int32_t state );

    entity_move_state move_state( void ) const { return mMoveState; }

    void flip_move_state( void );

    virtual void sync( void );
};

INLINE void entity::flip_move_state( void )
{
    switch ( mMoveState )
    {
        case entity_move_state::automatic: mMoveState = entity_move_state::manual; break;
        case entity_move_state::manual: mMoveState = entity_move_state::automatic; break;
    }
}

INLINE void entity::orient_kinematic_to( const glm::vec3& p )
{
    if ( mKinematicEnt )
        mKinematicEnt->orient_to( p );
}

INLINE void entity::set_normal_body_activation_state( int32_t state )
{
    if ( mPhysEnt )
        mPhysEnt->set_activation_state( state );
}

