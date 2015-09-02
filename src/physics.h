#pragma once

#include "def.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

struct application;
struct rigid_body;

const float INFINITE_MASS = 0.0f;

//-------------------------------------------------------------------------------------------------------
// util
//-------------------------------------------------------------------------------------------------------

static INLINE glm::vec3 get_collision_normal( const glm::vec3& normal, const rigid_body& a, const rigid_body& b );

//-------------------------------------------------------------------------------------------------------
// rigid_body
//-------------------------------------------------------------------------------------------------------

struct rigid_body
{
private:
    float mInvMass;

    glm::vec3
            mPosition,

            mInitialVelocity,

			mTorqueAccum,

			mForceAccum;

    glm::quat mOrientation;

	glm::mat3 mIitLocal, mIitWorld;

	uint32_t mOptions;

public:
    enum
    {
		RESET_POSITION = 0x1,
		RESET_VELOCITY = 0x2,
		RESET_FORCE_ACCUM = 0x4,
		RESET_ORIENTATION = 0x8,
		RESET_TORQUE_ACCUM = 0x10,

		LOCK_INTEGRATION = 0x20 // prevents integration from happening
    };

	rigid_body( uint32_t options = RESET_FORCE_ACCUM );

    void apply_force( const glm::vec3& force );

	void apply_force_at_local_point( const glm::vec3& f, const glm::vec3& point );

	void apply_force_at_point( const glm::vec3& f, const glm::vec3& point );

	void apply_velocity( const glm::vec3& initial );

    void integrate( float t );

    void reset( void );

    std::string info( void ) const;

    float mass( void ) const;

    float inv_mass( void ) const;

    const glm::vec3& position( void ) const;

    const glm::mat3 orientation( void ) const;

	const glm::mat3& iit_world( void ) const { return mIitWorld; }

    const glm::vec3& initial_velocity( void ) const;

	const glm::vec3& force_accum( void ) const;

	glm::vec3 acceleration( void ) const { return force_accum() * inv_mass(); }

    void mass( float m );

    void position( const glm::vec3& p );

    void position( uint32_t axis, float v );

    void orientation( const glm::mat4& mOrientation );

    void orientation( const glm::mat3& mOrientation );

    void set( const glm::mat4& t );

	void add_options( uint32_t flag ) { mOptions |= flag; }

	void remove_reset_bit( uint32_t flag ) { mOptions &= ~flag; }

	rigid_body from_velocity( const glm::vec3& v ) const;
};

INLINE const glm::vec3& rigid_body::position( void) const
{
    return mPosition;
}

INLINE const glm::mat3 rigid_body::orientation( void ) const
{
    return glm::mat3_cast( mOrientation );
}


INLINE const glm::vec3& rigid_body::initial_velocity( void ) const
{
	return mInitialVelocity;
}

INLINE const glm::vec3& rigid_body::force_accum( void ) const
{
	return mForceAccum;
}

INLINE void rigid_body::apply_force( const glm::vec3& force )
{
    mForceAccum += force;
}

INLINE void rigid_body::apply_force_at_local_point( const glm::vec3& f, const glm::vec3& point )
{
	glm::vec3 worldP( mOrientation * point );

	apply_force_at_point( f, worldP );
}

INLINE void rigid_body::apply_force_at_point( const glm::vec3& f, const glm::vec3& point )
{
	mForceAccum += f;
	mTorqueAccum += glm::cross( point, f );
}

INLINE void rigid_body::apply_velocity( const glm::vec3& v )
{
    mInitialVelocity += v;
}

INLINE void rigid_body::position( const glm::vec3& p )
{
    mPosition = p;
}

INLINE void rigid_body::position( uint32_t axis, float v )
{
    assert( axis < 3 );

    mPosition[ axis ] = v;
}

INLINE void rigid_body::orientation( const glm::mat4& orientation )
{
    this->mOrientation = std::move( glm::quat_cast( orientation ) );
}

INLINE void rigid_body::orientation( const glm::mat3& orientation )
{
    this->mOrientation = std::move( glm::quat_cast( orientation ) );
}

INLINE void rigid_body::set( const glm::mat4& t )
{
    mPosition = glm::vec3( t[ 3 ] );
    mOrientation = std::move( glm::quat_cast( t ) );
}

//-------------------------------------------------------------------------------------------------------
// physics_world
//-------------------------------------------------------------------------------------------------------

struct entity;

struct physics_world
{
private:
    std::vector< entity* > mBodies;

public:
    float mTime;

    const float mDT;

    float mT;

    uint32_t mLastMeasureCount;

    physics_world( float mTime, float mDT );

    void update( application& game );

    void sync_bodies( void );

    void clear_accum( void );
};

#include "physics.inl"


