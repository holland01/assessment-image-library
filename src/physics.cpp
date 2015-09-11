#include "physics.h"
#include "base.h"
#include "application.h"
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

rigid_body::rigid_body( uint32_t options )
    : mInvMass( 0.0f ),
      mLinearDamping( 1.0f ),
      mAngularDamping( 1.0f ),
      mPosition( 0.0f ),
      mLinearVelocity( 0.0f ),
      mAngularVelocity( 0.0f ),
      mLastAngularAccel( 0.0f ),
      mLastLinearAccel( 0.0f ),
	  mTorqueAccum( 0.0f ),
      mForceAccum( 0.0f ),
	  mIitLocal( 1.0f ),
	  mIitWorld( 1.0f ),
	  mOptions( options )
{
}

void rigid_body::integrate( float t )
{
	if ( mOptions & LOCK_INTEGRATION )
	{
		return;
	}

    mAngularVelocity += mLastAngularAccel * t;

    glm::vec3 accel( mLastLinearAccel * t );
    mLinearVelocity += accel;

    mLinearVelocity *= glm::pow( mLinearDamping, t );
    mAngularVelocity *= glm::pow( mAngularDamping, t );

    glm::vec3 add( mAngularVelocity * t );

    mOrientation += add;

    mOrientation = glm::normalize( mOrientation );

    mPosition += mOrientation * mLinearVelocity + accel * t;

    inertia_tensor_to_world( mIitWorld, mIitLocal, glm::mat3_cast( mOrientation ) );

    mLastLinearAccel = linear_acceleration();
    mLastAngularAccel = angular_acceleration();
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
       "\n Velocity: " << glm::to_string( mLinearVelocity ) <<
       "\n Force Accum: " << glm::to_string( mForceAccum ) <<
       "\n Mass: " << ( mInvMass == INFINITE_MASS? 0.0f: 1.0f / mInvMass );

	return ss.str();
}

void rigid_body::reset( void )
{
	if ( mOptions & RESET_FORCE_ACCUM ) mForceAccum = glm::zero< glm::vec3 >();
    if ( mOptions & RESET_TORQUE_ACCUM ) mTorqueAccum = glm::zero< glm::vec3 >();
    if ( mOptions & RESET_VELOCITY ) mLinearVelocity = glm::zero< glm::vec3 >();
	if ( mOptions & RESET_ORIENTATION ) mOrientation = glm::quat();
	if ( mOptions & RESET_POSITION ) mPosition = glm::zero< glm::vec3 >();
}

//-------------------------------------------------------------------------------------------------------
// world_t
//-------------------------------------------------------------------------------------------------------

physics_world::physics_world( float time_, float dt_ )
    : mTime( time_ ),
      mTargetDeltaTime( dt_ ),
      mT( 0.0f )
{
}

void physics_world::sync_bodies( void )
{
    float measure = mTargetDeltaTime;

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
