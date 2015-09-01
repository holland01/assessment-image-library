#include "physics.h"
#include "base.h"
#include "game.h"
#include <sstream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/projection.hpp>

namespace {

/*


static inline void _transformInertiaTensor(Matrix3 &iitWorld,
										   const Quaternion &q,
										   const Matrix3 &iitBody,
										   const Matrix4 &rotmat)
{
	real t4 = rotmat.data[0]*iitBody.data[0]+
		rotmat.data[1]*iitBody.data[3]+
		rotmat.data[2]*iitBody.data[6];

	real t9 = rotmat.data[0]*iitBody.data[1]+
		rotmat.data[1]*iitBody.data[4]+
		rotmat.data[2]*iitBody.data[7];

	real t14 = rotmat.data[0]*iitBody.data[2]+
		rotmat.data[1]*iitBody.data[5]+
		rotmat.data[2]*iitBody.data[8];

	real t28 = rotmat.data[4]*iitBody.data[0]+
		rotmat.data[5]*iitBody.data[3]+
		rotmat.data[6]*iitBody.data[6];

	real t33 = rotmat.data[4]*iitBody.data[1]+
		rotmat.data[5]*iitBody.data[4]+
		rotmat.data[6]*iitBody.data[7];

	real t38 = rotmat.data[4]*iitBody.data[2]+
		rotmat.data[5]*iitBody.data[5]+
		rotmat.data[6]*iitBody.data[8];

	real t52 = rotmat.data[8]*iitBody.data[0]+
		rotmat.data[9]*iitBody.data[3]+
		rotmat.data[10]*iitBody.data[6];

	real t57 = rotmat.data[8]*iitBody.data[1]+
		rotmat.data[9]*iitBody.data[4]+
		rotmat.data[10]*iitBody.data[7];

	real t62 = rotmat.data[8]*iitBody.data[2]+
		rotmat.data[9]*iitBody.data[5]+
		rotmat.data[10]*iitBody.data[8];

	iitWorld.data[0] = t4*rotmat.data[0]+
		t9*rotmat.data[1]+
		t14*rotmat.data[2];
	iitWorld.data[1] = t4*rotmat.data[4]+
		t9*rotmat.data[5]+
		t14*rotmat.data[6];
	iitWorld.data[2] = t4*rotmat.data[8]+
		t9*rotmat.data[9]+
		t14*rotmat.data[10];
	iitWorld.data[3] = t28*rotmat.data[0]+
		t33*rotmat.data[1]+
		t38*rotmat.data[2];
	iitWorld.data[4] = t28*rotmat.data[4]+
		t33*rotmat.data[5]+
		t38*rotmat.data[6];
	iitWorld.data[5] = t28*rotmat.data[8]+
		t33*rotmat.data[9]+
		t38*rotmat.data[10];
	iitWorld.data[6] = t52*rotmat.data[0]+
		t57*rotmat.data[1]+
		t62*rotmat.data[2];
	iitWorld.data[7] = t52*rotmat.data[4]+
		t57*rotmat.data[5]+
		t62*rotmat.data[6];
	iitWorld.data[8] = t52*rotmat.data[8]+
		t57*rotmat.data[9]+
		t62*rotmat.data[10];
}
*/


	// 4x4:

	// 0, 0 -> 0
	// 0, 1 -> 1
	// 0, 2 -> 2
	// 0, 3 -> 3
	// 1, 0 -> 4
	// 1, 1 -> 5
	// 1, 2 -> 6
	// 1, 3 -> 7
	// 2, 0 -> 8
	// 2, 1 -> 9
	// 2, 2 -> 10
	// 2, 3 -> 11
	// 3, 0 -> 12
	// 3, 1 -> 13
	// 3, 2 -> 14
	// 3, 3 -> 15

	// 3x3:

	// 0, 0 -> 0
	// 0, 1 -> 1
	// 0, 2 -> 2
	// 1, 0 -> 3
	// 1, 1 -> 4
	// 1, 2 -> 5
	// 2, 0 -> 6
	// 2, 1 -> 7
	// 2, 2 -> 8

INLINE void inertia_tensor_to_world( glm::mat3& iitWorld,
									 const glm::mat3& iitBody,
									 const glm::mat3& rotmat )
{
	float t4 = rotmat[ 0 ][ 0 ] * iitBody[ 0 ][ 0 ] +
		rotmat[ 0 ][ 1 ] * iitBody[ 1 ][ 0 ] +
		rotmat[ 0 ][ 2 ] * iitBody[ 2 ][ 0 ];

	float t9 = rotmat[ 0 ][ 0 ] * iitBody[ 0 ][ 1 ] +
		rotmat[ 0 ][ 1 ] * iitBody[ 1 ][ 1 ] +
		rotmat[ 0 ][ 2 ] * iitBody[ 2 ][ 1 ];

	float t14 = rotmat[ 0 ][ 0 ] * iitBody[ 0 ][ 2 ] +
		rotmat[ 0 ][ 1 ] * iitBody[ 1 ][ 2 ] +
		rotmat[ 0 ][ 2 ] * iitBody[ 2 ][ 2 ];

	float t28 = rotmat[ 1 ][ 0 ] * iitBody[ 0 ][ 0 ] +
		rotmat[ 1 ][ 1 ] * iitBody[ 1 ][ 0 ] +
		rotmat[ 1 ][ 2 ] * iitBody[ 2 ][ 0 ];

	float t33 = rotmat[ 1 ][ 0 ] * iitBody[ 0 ][ 1 ] +
		rotmat[ 1 ][ 1 ] * iitBody[ 1 ][ 1 ] +
		rotmat[ 1 ][ 2 ] * iitBody[ 2 ][ 1 ];

	float t38 = rotmat[ 1 ][ 0 ] * iitBody[ 0 ][ 2 ] +
		rotmat[ 1 ][ 1 ] * iitBody[ 1 ][ 2 ] +
		rotmat[ 1 ][ 2 ] * iitBody[ 2 ][ 2 ];

	float t52 = rotmat[ 2 ][ 0 ] * iitBody[ 0 ][ 0 ] +
		rotmat[ 2 ][ 1 ] * iitBody[ 1 ][ 0 ] +
		rotmat[ 2 ][ 2 ] * iitBody[ 2 ][ 0 ];

	float t57 = rotmat[ 2 ][ 0 ] * iitBody[ 0 ][ 1 ] +
		rotmat[ 2 ][ 1 ] * iitBody[ 1 ][ 1 ] +
		rotmat[ 2 ][ 2 ] * iitBody[ 2 ][ 1 ];

	float t62 = rotmat[ 2 ][ 0 ] * iitBody[ 0 ][ 2 ] +
		rotmat[ 2 ][ 1 ] * iitBody[ 1 ][ 2 ] +
		rotmat[ 2 ][ 2 ] * iitBody[ 2 ][ 2 ];

	iitWorld[ 0 ][ 0 ] = t4 * rotmat[ 0 ][ 0 ] +
		t9 * rotmat[ 0 ][ 1 ] +
		t14 * rotmat[ 0 ][ 2 ];

	iitWorld[ 0 ][ 1 ] = t4 * rotmat[ 1 ][ 0 ] +
		t9 * rotmat[ 1 ][ 1 ] +
		t14 * rotmat[ 1 ][ 2 ];

	iitWorld[ 0 ][ 2 ] = t4 * rotmat[ 2 ][ 0 ] +
		t9 * rotmat[ 2 ][ 1 ] +
		t14 * rotmat[ 2 ][ 2 ];

	iitWorld[ 1 ][ 0 ] = t28 * rotmat[ 0 ][ 0 ] +
		t33 * rotmat[ 0 ][ 1 ] +
		t38 * rotmat[ 0 ][ 2 ];

	iitWorld[ 1 ][ 1 ] = t28 * rotmat[ 1 ][ 0 ] +
		t33 * rotmat[ 1 ][ 1 ] +
		t38 * rotmat[ 1 ][ 2 ];

	iitWorld[ 1 ][ 2 ] = t28 * rotmat[ 2 ][ 0 ] +
		t33 * rotmat[ 2 ][ 1 ] +
		t38 * rotmat[ 2 ][ 2 ];

	iitWorld[ 2 ][ 0 ] = t52 * rotmat[ 0 ][ 0 ] +
		t57 * rotmat[ 0 ][ 1 ] +
		t62 * rotmat[ 0 ][ 2 ];

	iitWorld[ 2 ][ 1 ] = t52 * rotmat[ 1 ][ 0 ] +
		t57 * rotmat[ 1 ][ 1 ] +
		t62 * rotmat[ 1 ][ 2 ];

	iitWorld[ 2 ][ 2 ] = t52 * rotmat[ 2 ][ 0 ] +
		t57 * rotmat[ 2 ][ 1 ] +
		t62 * rotmat[ 2 ][ 2 ];
}
}

//-------------------------------------------------------------------------------------------------------
// body_t
//-------------------------------------------------------------------------------------------------------

rigid_body::rigid_body( uint32_t resetBits_ )
    : mInvMass( 0.0f ),
      mPosition( 0.0f ),
      mInitialVelocity( 0.0f ),
	  mTorqueAccum( 0.0f ),
      mForceAccum( 0.0f ),
	  mIitLocal( 1.0f ),
	  mIitWorld( 1.0f ),
	  mResetBits( resetBits_ )
{
}

void rigid_body::integrate( float t )
{
	inertia_tensor_to_world( mIitWorld, mIitLocal, glm::mat3_cast( mOrientation ) );

	glm::vec3 accel( acceleration() );

    glm::mat3 orient( glm::mat3_cast( mOrientation ) );

	glm::vec3 v( orient * mInitialVelocity + accel * t );

	mPosition += v * t;
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
	if ( mResetBits & RESET_TORQUE_ACCUM_BIT ) mTorqueAccum = glm::zero< glm::vec3 >();
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
