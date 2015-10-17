#pragma once

#include "def.h"
#include <memory>
#include <vector>
#include <bullet3/btBulletCollisionCommon.h>
#include <bullet3/btBulletDynamicsCommon.h>
#include "physics_body.h"

struct physics_world
{
    friend struct physics_body;

    using rigid_info_t = btRigidBody::btRigidBodyConstructionInfo;

private:

    std::unique_ptr< btBroadphaseInterface > mBroadphase;
    std::unique_ptr< btDefaultCollisionConfiguration > mCollisionConfig;
    std::unique_ptr< btCollisionDispatcher > mDispatcher;
    std::unique_ptr< btSequentialImpulseConstraintSolver > mSolver;
    std::unique_ptr< btDiscreteDynamicsWorld > mDynamics;

    std::vector< const physics_body* > mPhysEntities;

    physics_body mGround;

public:

    static constexpr float TIME_STEP = 1.0f / 60.0f;

    physics_world( void );

    void step( void );

    void clear_physics_entities( void );
};
