#include "physics.h"
#include "base.h"
#include <sstream>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/projection.hpp>

namespace phys {

body_t::body_t( void )
	: invMass( 0.0f ),
	  position( 0.0f ),
	  velocity( 0.0f ),
	  forceAccum( 0.0f ),
	  initialForce( 0.0f ),
	  orientation( 1.0f )
{
}

void body_t::Integrate( float t )
{
	glm::vec3 accel( forceAccum * invMass );

	glm::vec3 v( orientation * velocity + accel * t );
	position += v * t;
}

std::string body_t::Info( void ) const
{
	std::stringstream ss;

	ss << "Position: " << glm::to_string( position ) <<
	   "\n Velocity: " << glm::to_string( velocity ) <<
	   "\n Force Accum: " << glm::to_string( forceAccum ) <<
	   "\n Mass: " << ( invMass == INFINITE_MASS? 0.0f: 1.0f / invMass );

	return ss.str();
}

void body_t::Reset( void )
{
	forceAccum = initialForce;
}

}
