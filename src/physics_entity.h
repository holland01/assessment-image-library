#pragma once

#include "def.h"

#include <memory>
#include <bullet3/btBulletCollisionCommon.h>
#include <bullet3/btBulletDynamicsCommon.h>
#include <functional>
#include "glm_ext.hpp"

struct imm_draw;

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

    void draw( imm_draw& drawer, uint32_t primType ) const;
};

