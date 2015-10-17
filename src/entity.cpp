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

void entity::toggle_kinematic( void )
{
    if ( !mKinematicEnt )
    {
        btCollisionShape* box = new btBoxShape( mPhysEnt->box_shape()->getHalfExtentsWithMargin() );

        btTransform t;
        mPhysEnt->motion_state().getWorldTransform( t );

        btMotionState* ms = new btDefaultMotionState( t );

        btRigidBody* kinematicBody = new btRigidBody(
                    btRigidBody::btRigidBodyConstructionInfo( mPhysEnt->mass(),
                                                              ms,
                                                              box ) );

        mKinematicEnt.reset( new physics_body( box,
                                                 ms,
                                                 kinematicBody ) );

        mKinematicEnt->toggle_kinematic();
        mKinematicEnt->mBody->setIgnoreCollisionCheck( mPhysEnt->mBody.get(), true );
        mKinematicEnt->add_to_world();
    }
}

void entity::sync( void )
{
    if ( !mPhysEnt || !mKinematicEnt )
        return;

    auto do_update = [ this ]( physics_body* dest, const physics_body* src ) -> void
    {
        btTransform tSrc;
        src->motion_state().getWorldTransform( tSrc );

        btTransform tDest;
        dest->motion_state().getWorldTransform( tDest );
        tDest.setOrigin( tSrc.getOrigin() );
        dest->motion_state().setWorldTransform( tDest );
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
