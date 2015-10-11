#pragma once

#include "def.h"

#include <memory>
#include <bullet3/btBulletCollisionCommon.h>
#include <bullet3/btBulletDynamicsCommon.h>
#include <functional>

struct physics_entity
{
    friend struct entity;

private:
    bool mOwned;

    std::unique_ptr< btCollisionShape > mShape;
    std::unique_ptr< btRigidBody > mBody;
    std::unique_ptr< btDefaultMotionState > mMotionState;

public:
    physics_entity( btCollisionShape* shape, btRigidBody* body, btDefaultMotionState* ms )
        : mOwned( false ),
          mShape( shape ),
          mBody( body ),
          mMotionState( ms )
    {
    }

    physics_entity( void )
        : physics_entity( nullptr, nullptr, nullptr )
    {
    }

    btRigidBody* body( void ) { return mBody.get(); }
};

