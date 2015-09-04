#pragma once

#include "def.h"
#include <glm/glm.hpp>
#include <glm/vec3.hpp>

namespace glm {

namespace ext {

using index_t = glm::int_t;

const index_t no_index = -1;

INLINE index_t index_of( float x, const glm::vec3& v )
{
    if ( x == v.x ) return 0;
    if ( x == v.y ) return 1;
    if ( x == v.z ) return 2;

    return -1;
}

INLINE index_t max_index( const glm::vec3& v )
{
    return index_of( glm::max( v.x, glm::max( v.y, v.z ) ), v );
}

INLINE index_t min_index( const glm::vec3& v )
{
    return index_of( glm::min( v.x, glm::min( v.y, v.z ) ), v );
}

INLINE bool range( const glm::vec3& v, const glm::vec3& min, const glm::vec3& max, index_t i )
{
	return min[ i ] <= v[ i ] && v[ i ] <= max[ i ];
}

INLINE glm::mat3 project_cardinal( const glm::mat3& m, index_t normalAxis )
{
    glm::mat3 copy( m );
    copy[ normalAxis ][ 0 ] = 0.0f;
    copy[ normalAxis ][ 1 ] = 0.0f;
    copy[ normalAxis ][ 2 ] = 0.0f;

    return std::move( copy );
}

INLINE glm::vec3 project_cardinal( const glm::vec3& v, index_t normalAxis )
{
    glm::vec3 copy( v );
    copy[ normalAxis ] = 0.0f;
    return std::move( copy );
}

// a and b are copied in the event that max and min are equivalent to a and b
// when this function is called
INLINE void maxmin( glm::vec3& max, glm::vec3& min, glm::vec3 a, glm::vec3 b )
{
    max = glm::max( a, b );
    min = glm::min( a, b );
}

} // ext

} // glm
