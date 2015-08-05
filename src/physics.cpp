#include "physics.h"
#include "base.h"
#include "game.h"
#include <sstream>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/projection.hpp>

//-------------------------------------------------------------------------------------------------------
// body_t
//-------------------------------------------------------------------------------------------------------

body_t::body_t( uint32_t resetBits_ )
	: invMass( 0.0f ),
	  position( 0.0f ),
      initialVelocity( 0.0f ),
	  forceAccum( 0.0f ),
	  initialForce( 0.0f ),
      totalVelocity( 0.0f ),
      orientation( 1.0f ),
      resetBits( resetBits_ )
{
}

void body_t::Integrate( float t )
{
    glm::vec3 accel( forceAccum * invMass );
    totalVelocity = orientation * initialVelocity + accel * t;
    position += totalVelocity * t;
}

void body_t::SetMass( float m )
{
    if ( m == INFINITE_MASS )
    {
        invMass = INFINITE_MASS;
        return;
    }

    invMass = 1.0f / m;
}

float body_t::GetMass( void ) const
{
    if ( invMass == INFINITE_MASS )
    {
        return INFINITE_MASS;
    }

    return 1.0f / invMass;
}

float body_t::GetInvMass( void ) const
{
    return invMass;
}

std::string body_t::GetInfoString( void ) const
{
	std::stringstream ss;

	ss << "Position: " << glm::to_string( position ) <<
       "\n Velocity: " << glm::to_string( initialVelocity ) <<
	   "\n Force Accum: " << glm::to_string( forceAccum ) <<
	   "\n Mass: " << ( invMass == INFINITE_MASS? 0.0f: 1.0f / invMass );

	return ss.str();
}

void body_t::Reset( void )
{
    if ( resetBits & RESET_FORCE_ACCUM_BIT ) forceAccum = glm::zero< glm::vec3 >();
    if ( resetBits & RESET_VELOCITY_BIT ) initialVelocity = glm::zero< glm::vec3 >();
    if ( resetBits & RESET_ORIENTATION_BIT ) orientation = glm::one< glm::mat3 >();
    if ( resetBits & RESET_POSITION_BIT ) position = glm::zero< glm::vec3 >();
}

//-------------------------------------------------------------------------------------------------------
// world_t
//-------------------------------------------------------------------------------------------------------

world_t::world_t( float time_, float dt_ )
	: time( time_ ),
      dt( dt_ ),
      t( 0.0f )
{
}

void world_t::Update( game_t& game )
{
    game.world.bodies.push_back( game.camera->body );

    if ( !game.drawAll )
    {
        for ( const tile_t* t: game.walls )
        {
            game.world.bodies.push_back( t->body );
        }
    }
    else
    {
        for ( const tile_t* t: game.gen->walls )
        {
            game.world.bodies.push_back( t->body );
        }
    }

    game.camera->ApplyMovement();

	float measure = time;

	while ( measure > 0.0f )
	{
		float delta = glm::min( measure, dt );

        for ( std::weak_ptr< body_t >& body: bodies )
		{
            auto p = body.lock();

            if ( p )
            {
                p->Integrate( dt );
            }
		}

		measure -= delta;
		t += delta;
	}

    game.camera->Sync();

    ClearAllAccum();
}

void world_t::ClearAllAccum( void )
{
    for ( std::weak_ptr< body_t >& body: bodies )
    {
        auto p = body.lock();

        if ( p )
        {
            p->Reset();
        }
    }
}
