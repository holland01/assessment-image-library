#include "entity.h"
#include "geom/geom.h"
#include "physics_world.h"

//-------------------------------------------------------------------------------------------------------
// entity_bounds_primitive_t
//-------------------------------------------------------------------------------------------------------

entity::entity( const glm::vec4& color )
    : mPhysEnt( nullptr ),
      mKinematicEnt( nullptr ),
      mMoveState( entity_move_state::automatic ),
      mColor( color ),
      mSize( 1.0f )
{
}

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
