#pragma once

#include "def.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <memory>
#include <vector>

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

            mAngularVelocity,

            mForceAccum,

            mInitialForce,

            mTotalVelocity;

    glm::quat mOrientation;

    uint32_t mResetBits;

public:
    enum
    {
        RESET_POSITION_BIT = 0x1,
        RESET_VELOCITY_BIT = 0x2,
        RESET_FORCE_ACCUM_BIT = 0x4,
        RESET_ORIENTATION_BIT = 0x8,
        RESET_ANGULAR_VELOCITY_BIT = 0x10
    };

    rigid_body( uint32_t mResetBits = RESET_FORCE_ACCUM_BIT );

    void angular_velocity( const glm::vec3& av );

    void apply_force( const glm::vec3& force );

    void apply_velocity( const glm::vec3& mInitialVelocity );

    void integrate( float t );

    void reset( void );

    std::string info( void ) const;

    float mass( void ) const;

    float inv_mass( void ) const;

    const glm::vec3& position( void ) const;

    const glm::mat3 orientation( void ) const;

    const glm::vec3& total_velocity( void ) const;

    const glm::vec3& initial_velocity( void ) const;

    void mass( float m );

    void position( const glm::vec3& p );

    void position( uint32_t axis, float v );

    void orientation( const glm::mat4& mOrientation );

    void orientation( const glm::mat3& mOrientation );

    void set( const glm::mat4& t );
};

INLINE const glm::vec3& rigid_body::position( void) const
{
    return mPosition;
}

INLINE const glm::mat3 rigid_body::orientation( void ) const
{
    return glm::mat3_cast( mOrientation );
}

INLINE const glm::vec3& rigid_body::total_velocity( void ) const
{
    return mTotalVelocity;
}

INLINE const glm::vec3& rigid_body::initial_velocity( void ) const
{
    return mInitialVelocity;
}

INLINE void rigid_body::apply_force( const glm::vec3& force )
{
    mForceAccum += force;
}

INLINE void rigid_body::apply_velocity( const glm::vec3& v )
{
    mInitialVelocity += v;
}

INLINE void rigid_body::angular_velocity( const glm::vec3& v )
{
    mAngularVelocity = v;
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

struct physics_world
{
    std::vector< std::weak_ptr< rigid_body > > mBodies;

    float mTime;

    const float mDT;

    float mT;

    uint32_t mLastMeasureCount;

    physics_world( float mTime, float mDT );

    void update( application& game );

    void clear_accum( void );
};

#include "physics.inl"


