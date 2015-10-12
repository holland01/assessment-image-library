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
                                              mCollisionConfig.get() ) ),
      mGround( std::move( []( void ) -> physics_entity
               {
                    btCollisionShape* plane = new btStaticPlaneShape( btVector3( 0.0f, 1.0f, 0.0f ), 1.0f );
                    btDefaultMotionState* ms = new btDefaultMotionState(
                                btTransform( btQuaternion( 0.0f, 0.0f, 0.0f, 1.0f ),
                                             btVector3( 0.0f, -1.0f, 0.0f ) ) );

                    btRigidBody* b = new btRigidBody(
                                btRigidBody::btRigidBodyConstructionInfo( 0.0f, ms, plane ) );

                    return std::move( physics_entity( plane, ms, b ) );
               }() ) )
{
    mDynamics->setGravity( btVector3( 0, -1.0f, 0 ) );
    mDynamics->setSynchronizeAllMotionStates( true );
    mDynamics->addRigidBody( mGround.body() );
}

void physics_world::step( void )
{
    mDynamics->stepSimulation( TIME_STEP, 10 );
}

void physics_world::clear_physics_entities( void )
{
    mPhysEntities.clear();
}