#pragma once

#include "_geom_local.h"

struct ray
{
	glm::vec3 p;
	glm::vec3 d;
	float t;

	ray( const glm::vec3& position = glm::vec3( 0.0f ),
		 const glm::vec3& dir = glm::vec3( 0.0f ),
		 float t_ = 1.0f )
		: p( position ),
		  d( dir ),
		  t( t_ )
	{}

	ray( ray&& ) = delete;
	ray& operator=( ray&& ) = delete;

	ray( const ray& r )
		: p( r.p ),
		  d( r.d ),
		  t( r.t )
	{
	}

	ray& operator=( const ray& r )
	{
		if ( this != &r )
		{
			p = r.p;
			d = r.d;
			t = r.t;
		}

		return *this;
	}

	glm::vec3 calc_position( void ) const
	{
		return p + d * t;
	}
};

