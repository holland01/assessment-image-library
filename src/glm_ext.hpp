#pragma once

#include "def.h"
#include <glm/glm.hpp>
#include <glm/vec3.hpp>

namespace glmext {

using index_t = glm::int_t;

const int32_t no_index = -1;

INLINE index_t max_index( const glm::vec3& v )
{
	float x = glm::max( v.x, glm::max( v.y, v.z ) );

	if ( x == v.x ) return 0;
	if ( x == v.y ) return 1;
	if ( x == v.z ) return 2;

	return -1;
}

INLINE bool in_range( const glm::vec3& v, const glm::vec3& min, const glm::vec3& max, index_t i )
{
	return min[ i ] <= v[ i ] && v[ i ] <= max[ i ];
}

}

