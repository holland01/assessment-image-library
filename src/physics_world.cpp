#include "physics_world.h"
#include <assert.h>

physics_world::physics_world( void )
    : mBroadphase( new btDbvtBroadphase() ),
      mCollisionConfig( new btDefaultCollisionConfiguration() ),
      mDispatcher( new btCollisionDispatcher( mCollisionConfig.get() ) ),
      mSolver( new btSequentialImpulseConstraintSolver() ),
      mDynamics( new btDiscreteDynamicsWorld( mDispatcher.get(),
                                              mBroadphase.get(),
                                              mSolver.get(),
                                              mCollisionConfig.get() ) )
{
    mDynamics->setGravity( btVector3( 0, -1.0f, 0 ) );
}

void physics_world::step( void )
{
    mDynamics->stepSimulation( TIME_STEP, 10 );
}

void physics_world::remove_bodies( void )
{
    for ( physics_entity* e: mPhysEntities )
    {
        assert( e );
        mDynamics->removeRigidBody( e->body() );
    }

    mPhysEntities.clear();
}
