#pragma once

#include "def.h"

#include <memory>
#include <bullet3/btBulletCollisionCommon.h>
#include <bullet3/btBulletDynamicsCommon.h>
#include <functional>
#include "glm_ext.hpp"

struct imm_draw;
struct view_data;

struct physics_entity
{
    friend struct entity;

private:
    bool mOwned;

    std::unique_ptr< btCollisionShape > mShape;

    std::unique_ptr< btDefaultMotionState > mMotionState;

    std::unique_ptr< btRigidBody > mBody;

public:
    physics_entity( btCollisionShape* shape, btDefaultMotionState* ms, btRigidBody* body );

    physics_entity( float mass, const glm::mat4& orientAndTranslate, const glm::vec3& halfSpaceExtents );

    physics_entity( void );

    const btBoxShape* shape_as_box( void ) const { return ( const btBoxShape* ) mShape.get(); }

    btRigidBody* body( void ) { return mBody.get(); }

    const btDefaultMotionState& motion_state( void ) const { return *mMotionState; }

    glm::mat4 world_transform( void ) const;

    void draw( imm_draw& drawer, uint32_t primType ) const;

    void draw( const std::string& buffer,
               const std::string& program,
               const view_data& view,
               const glm::vec4& color ) const;
};

INLINE glm::mat4 physics_entity::world_transform( void ) const
{
    assert( mMotionState );

    btTransform t;
    // TODO: incorporate scale of the world transform
    // for specific collision shapes ( e.g., physics_entities using btBoxShape, and its halfSpaceExtents )
    mMotionState->getWorldTransform( t );
    return glm::ext::from_bullet( t );
}
