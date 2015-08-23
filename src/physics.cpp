#include "physics.h"
#include "base.h"
#include "game.h"
#include <sstream>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/projection.hpp>

//-------------------------------------------------------------------------------------------------------
// body_t
//-------------------------------------------------------------------------------------------------------

rigid_body::rigid_body( uint32_t resetBits_ )
    : mInvMass( 0.0f ),
      mPosition( 0.0f ),
      mInitialVelocity( 0.0f ),
      mForceAccum( 0.0f ),
      mInitialForce( 0.0f ),
      mTotalVelocity( 0.0f ),
      mOrientation( 1.0f ),
      mResetBits( resetBits_ )
{
}

void rigid_body::integrate( float t )
{
    glm::vec3 accel( mForceAccum * mInvMass );
    mTotalVelocity = mOrientation * mInitialVelocity + accel * t;
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
    if ( mResetBits & RESET_ORIENTATION_BIT ) mOrientation = glm::one< glm::mat3 >();
    if ( mResetBits & RESET_POSITION_BIT ) mPosition = glm::zero< glm::vec3 >();
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

void physics_world::update( application& game )
{
    mBodies.push_back( game.camera->mBody );

    const map_tile_list_t& walls = ( game.drawAll )? game.gen->walls(): game.walls;
    for ( const map_tile* t: walls )
    {
        mBodies.push_back( t->mBody );
    }

    if ( game.bullet )
    {
        mBodies.push_back( game.bullet->mBody );
    }

    game.camera->ApplyMovement();

    float measure = mDT;

    mLastMeasureCount = 0;

	while ( measure > 0.0f )
	{
        float delta = glm::max( glm::min( mTime, measure ), 0.1f );

        for ( std::weak_ptr< rigid_body >& body: mBodies )
		{
            auto p = body.lock();

            if ( p )
            {
                p->integrate( delta );
            }
		}

        measure -= delta;
        mT += delta;
        mLastMeasureCount++;
    }

    if ( game.bullet )
    {
        game.bullet->sync();
    }

    game.camera->sync();

    clear_accum();
}

void physics_world::clear_accum( void )
{
    for ( std::weak_ptr< rigid_body >& body: mBodies )
    {
        auto p = body.lock();

        if ( p )
        {
            p->reset();
        }
    }
}
