#pragma once

#include "def.h"
#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <stdint.h>

namespace glm {

namespace ext {

template < typename vec_type >
struct maxmin_pair
{
    vec_type max = vec_type( 0.0f );
    vec_type min = vec_type( 0.0f );
};

enum class cardinal_plane
{
    x = 0,
    y = 1,
    z = 2
};

using vec3_maxmin_pair_t = maxmin_pair< glm::vec3 >;

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

INLINE glm::vec3 project_cardinal( const glm::vec3& v, cardinal_plane p )
{
    glm::vec3 copy( v );
    copy[ ( index_t ) p ] = 0.0f;
    return std::move( copy );
}

// a and b are copied in the event that max and min are equivalent to a and b
// when this function is called
INLINE void maxmin( glm::vec3& max, glm::vec3& min, glm::vec3 a, glm::vec3 b )
{
    max = glm::max( a, b );
    min = glm::min( a, b );
}

INLINE cardinal_plane best_cardinal_plane( const glm::vec3& normal )
{
    //assert( glm::length( normal ) == 1.0f && "normal must be normalized" );

    glm::mat3 cardinal( 1.0f );

    uint32_t index = 0;
    float max = 0.0f;

    for ( uint32_t i = 0; i < 3; ++i )
    {
        float d = glm::abs( glm::dot( normal, cardinal[ i ] ) );

        if ( d > max )
        {
            max = d;
            index = i;
        }
    }

    return ( cardinal_plane )( index );
}

template < typename list_type >
INLINE vec3_maxmin_pair_t maxmin_from_list( const list_type& list )
{
    vec3_maxmin_pair_t pair;

    glm::vec3 line( 0.0f );

    float maxLen = 0.0f;

    for ( const glm::vec3& p: list )
    {
        for ( const glm::vec3& p0: list )
        {
            if ( p0 == p )
            {
                continue;
            }

            glm::vec3 d( p0 - p );
            float dlen = glm::length( d );

            if ( dlen > maxLen )
            {
                line = std::move( d );
                maxLen = dlen;

                glm::ext::maxmin( pair.max, pair.min, p, p0 );
            }
        }
    }

    return std::move( pair );
}

} // ext

} // glm
