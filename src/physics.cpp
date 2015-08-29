#include "physics.h"
#include "base.h"
#include "game.h"
#include <sstream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/projection.hpp>

//-------------------------------------------------------------------------------------------------------
// body_t
//-------------------------------------------------------------------------------------------------------

rigid_body::rigid_body( uint32_t resetBits_ )
    : mInvMass( 0.0f ),
      mAngularRot( 0.0f ),
      mPosition( 0.0f ),
      mInitialVelocity( 0.0f ),
      mAngularAxis( 0.0f ),
      mForceAccum( 0.0f ),
      mInitialForce( 0.0f ),
      mTotalVelocity( 0.0f ),
      mResetBits( resetBits_ )
{
}

void rigid_body::integrate( float t )
{
    glm::vec3 accel( mForceAccum * mInvMass );

    if ( mAngularRot != 0.0f )
    {
        float rot = mAngularRot * 0.01f * t;

        mOrientation = glm::rotate( mOrientation, rot, glm::inverse( mOrientation ) * mAngularAxis );
    }

    glm::mat3 orient( glm::mat3_cast( mOrientation ) );

    mTotalVelocity = orient * mInitialVelocity + accel * t;

    mPosition += mTotalVelocity * t;
}

void rigid_body::mass( float m )
{
    if ( m == INFINITE_MASS )
    {
        mInvMass = INFINITE_MASS;
        return;
    }

    mInvMass = 1.0f / m;
}

float rigid_body::mass( void ) const
{
    if ( mInvMass == INFINITE_MASS )
    {
        return INFINITE_MASS;
    }

    return 1.0f / mInvMass;
}

float rigid_body::inv_mass( void ) const
{
    return mInvMass;
}

std::string rigid_body::info( void ) const
{
	std::stringstream ss;

    ss << "Position: " << glm::to_string( mPosition ) <<
       "\n Velocity: " << glm::to_string( mInitialVelocity ) <<
       "\n Force Accum: " << glm::to_string( mForceAccum ) <<
       "\n Mass: " << ( mInvMass == INFINITE_MASS? 0.0f: 1.0f / mInvMass );

	return ss.str();
}

void rigid_body::reset( void )
{
    if ( mResetBits & RESET_FORCE_ACCUM_BIT ) mForceAccum = glm::zero< glm::vec3 >();
    if ( mResetBits & RESET_VELOCITY_BIT ) mInitialVelocity = glm::zero< glm::vec3 >();
    if ( mResetBits & RESET_ORIENTATION_BIT ) mOrientation = glm::quat();
    if ( mResetBits & RESET_POSITION_BIT ) mPosition = glm::zero< glm::vec3 >();
    if ( mResetBits & RESET_ANGULAR_VELOCITY_BIT )
    {
        mAngularAxis = glm::zero< glm::vec3 >();
        mAngularRot = 0.0f;
    }
}

//-------------------------------------------------------------------------------------------------------
// world_t
//-------------------------------------------------------------------------------------------------------

physics_world::physics_world( float time_, float dt_ )
    : mTime( time_ ),
      mDT( dt_ ),
      mT( 0.0f )
{
}

namespace {
    INLINE void push_bodies( std::vector< entity* >& entities, map_tile_list_t tiles )
    {
        for ( map_tile* t: tiles )
        {
            entities.push_back( t );
        }
    }

    INLINE void update_billboards( application& g )
    {
        map_tile_list_t billboards = std::move( g.billboard_list() );

        for ( map_tile* t: billboards )
        {
            if ( g.billboard_oriented( *t ) )
            {
                t->orient_to( g.camera->view_params().mOrigin );
            }
        }
    }

}

void physics_world::sync_bodies( void )
{
    float measure = mDT;

    mLastMeasureCount = 0;

    while ( measure > 0.0f )
    {
        float delta = glm::max( glm::min( mTime, measure ), 0.1f );

        for ( entity* e: mBodies )
        {
            if ( e )
            {
                e->mBody->integrate( delta );
                e->sync();
            }
        }

        measure -= delta;
        mT += delta;
        mLastMeasureCount++;
    }
}

void physics_world::update( application& game )
{
    update_billboards( game );

    mBodies.clear();
    mBodies.push_back( game.camera );

    if ( game.bullet )
    {
        mBodies.push_back( game.bullet.get() );
    }

    push_bodies( mBodies, std::move( game.wall_list() ) );
    push_bodies( mBodies, std::move( game.billboard_list() ) );

    game.camera->apply_movement();

    sync_bodies();

    clear_accum();
}

void physics_world::clear_accum( void )
{
    for ( entity* e: mBodies )
    {
        if ( e )
        {
            e->mBody->reset();
        }
    }
}
