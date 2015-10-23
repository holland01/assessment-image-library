#include "entity.h"
#include "geom/geom.h"
#include "physics/physics_world.h"

#define MAX_ENTITIES ( 1 << 22 )

entity entity_manager::make_entity( void )
{
    uint32_t index;

    if ( mFreeIndices.size() > MIN_FREE_INDICES )
    {
        index = mFreeIndices.front();
        mFreeIndices.pop();
    }
    else
    {
        mGenerations.push_back( 0 );
        index = mGenerations.size() - 1;
        assert( index < MAX_ENTITIES );
    }

    entity e;
    e.mId = index | ( mGenerations[ index ] << ENTITY_INDEX_BITS );

    return e;
}



/*

DON'T DELETE: this is kept here for future reference

void entity::toggle_kinematic( bool addMotionState )
{
    if ( !mKinematicEnt )
    {
        btCollisionShape* box = new btBoxShape( mPhysEnt->box_shape()->getHalfExtentsWithMargin() );

        btTransform t;
        mPhysEnt->motion_state().getWorldTransform( t );

        btMotionState* ms = nullptr;

        if ( addMotionState )
            ms = new btDefaultMotionState( t );

        btRigidBody* kinematicBody = new btRigidBody(
                    btRigidBody::btRigidBodyConstructionInfo( mPhysEnt->mass(),
                                                              ms, box ) );

        if ( !addMotionState )
            kinematicBody->setCenterOfMassTransform( t );

        mKinematicEnt.reset( new physics_body( box,
                                                 ms,
                                                 kinematicBody ) );

        mKinematicEnt->toggle_kinematic();
        mKinematicEnt->mBody->setIgnoreCollisionCheck( mPhysEnt->mBody.get(), true );
    }
}


void entity::sync( void )
{
    if ( !mPhysEnt || !mKinematicEnt )
        return;

    auto do_update = [ this ]( physics_body* dest, const physics_body* src ) -> void
    {
        if ( dest->mMotionState && src->mMotionState )
        {
            btTransform tSrc;
            src->motion_state().getWorldTransform( tSrc );

            btTransform tDest;
            dest->motion_state().getWorldTransform( tDest );
            tDest.setOrigin( tSrc.getOrigin() );
            dest->motion_state().setWorldTransform( tDest );
        }
        else if ( dest->mMotionState )
        {
            btTransform tDest;
            dest->motion_state().getWorldTransform( tDest );
            tDest.setOrigin( src->mBody->getCenterOfMassPosition() );
            dest->motion_state().setWorldTransform( tDest );
        }
        else if ( src->mMotionState )
        {
            btTransform tSrc;
            src->motion_state().getWorldTransform( tSrc );
            dest->mBody->translate( tSrc.getOrigin() - dest->mBody->getCenterOfMassPosition() );
        }
        else
        {
            dest->mBody->translate( src->mBody->getCenterOfMassPosition() - dest->mBody->getCenterOfMassPosition() );
        }
    };

    switch ( mMoveState )
    {
        case entity_move_state::automatic:
        {
            do_update( mKinematicEnt.get(), mPhysEnt.get() );
        }
            break;

        case entity_move_state::manual:
        {
            do_update( mPhysEnt.get(), mKinematicEnt.get() );
        }
            break;
    }
}

*/


