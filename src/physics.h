#pragma once

#include "def.h"
#include <glm/glm.hpp>
#include <string>
#include <memory>
#include <vector>

struct game_t;
struct body_t;

const float INFINITE_MASS = 0.0f;

//-------------------------------------------------------------------------------------------------------
// util
//-------------------------------------------------------------------------------------------------------

static INLINE glm::vec3 P_GenericCollideNormal( const glm::vec3& normal, const body_t& a, const body_t& b );


//-------------------------------------------------------------------------------------------------------
// body_t
//-------------------------------------------------------------------------------------------------------

struct body_t
{
private:
	float invMass;
    glm::vec3 position,
              initialVelocity,
              forceAccum,
              initialForce,
              totalVelocity;

	glm::mat3 orientation;

    uint32_t resetBits;

public:
    enum
    {
        RESET_POSITION_BIT = 0x1,
        RESET_VELOCITY_BIT = 0x2,
        RESET_FORCE_ACCUM_BIT = 0x4,
        RESET_ORIENTATION_BIT = 0x8
    };

    body_t( uint32_t resetBits = RESET_FORCE_ACCUM_BIT );

    void ApplyForce( const glm::vec3& force );
    void ApplyVelocity( const glm::vec3& initialVelocity );

    void Integrate( float t );

	void Reset( void );

    std::string GetInfoString( void ) const;

    float GetMass( void ) const;
    float GetInvMass( void ) const;
    const glm::vec3& GetPosition( void ) const;
    const glm::mat3& GetOrientation( void ) const;
    const glm::vec3& GetTotalVelocity( void ) const;
    const glm::vec3& GetInitialVelocity( void ) const;

    void SetMass( float m );
    void SetCenter( const glm::vec3& p );
    void SetPositionAxis( uint32_t axis, float v );
    void SetOrientation( const glm::mat4& orientation );
    void SetFromTransform( const glm::mat4& t );
};

INLINE const glm::vec3& body_t::GetPosition( void) const
{
    return position;
}

INLINE const glm::mat3& body_t::GetOrientation( void ) const
{
    return orientation;
}

INLINE const glm::vec3& body_t::GetTotalVelocity( void ) const
{
    return totalVelocity;
}

INLINE const glm::vec3& body_t::GetInitialVelocity( void ) const
{
    return initialVelocity;
}

INLINE void body_t::ApplyForce( const glm::vec3& force )
{
    forceAccum += force;
}

INLINE void body_t::ApplyVelocity( const glm::vec3& v )
{
    initialVelocity += v;
}

INLINE void body_t::SetCenter( const glm::vec3& p )
{
    position = p;
}

INLINE void body_t::SetPositionAxis( uint32_t axis, float v )
{
    assert( axis < 3 );

    position[ axis ] = v;
}

INLINE void body_t::SetOrientation( const glm::mat4& orientation )
{
    this->orientation = std::move( glm::mat3( orientation ) );
}

INLINE void body_t::SetFromTransform( const glm::mat4& t )
{
    position = glm::vec3( t[ 3 ] );
    orientation = glm::mat3( t );
}

//-------------------------------------------------------------------------------------------------------
// body_t
//-------------------------------------------------------------------------------------------------------

struct world_t
{
    std::vector< std::weak_ptr< body_t > > bodies;

	float time;
	float dt;
    float t;

	world_t( float time, float dt );

	void Update( game_t& game );

    void ClearAllAccum( void );
};

#include "physics.inl"
