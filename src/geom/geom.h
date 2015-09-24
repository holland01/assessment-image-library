#pragma once

#include "../def.h"
#include "../renderer.h"
#include "../glm_ext.hpp"
#include "../collision_contact.h"
#include "plane.h"
#include "transform_data.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <stdint.h>
#include <stdio.h>
#include <memory>
#include <stack>
#include <unordered_set>

struct ray;

//-------------------------------------------------------------------------------------------------------
// util
//-------------------------------------------------------------------------------------------------------



//-------------------------------------------------------------------------------------------------------
// ray_t
//-------------------------------------------------------------------------------------------------------

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

#include "geom.inl"
