#pragma once

#include "def.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/simd_mat4.hpp>
#include <glm/gtx/simd_vec4.hpp>
#include <stdint.h>
#include <utility>
#include <functional>
#include <bullet3/LinearMath/btTransform.h>
#include <bullet3/LinearMath/btMatrix3x3.h>

namespace glm {

#if GLM_ARCH != GLM_ARCH_PURE
INLINE void mat3_to_simd( glm::simdMat4& out, const glm::mat3& in )
{
	out[ 0 ] = glm::simdVec4( in[ 0 ], 0.0f );
	out[ 1 ] = glm::simdVec4( in[ 1 ], 0.0f );
	out[ 2 ] = glm::simdVec4( in[ 2 ], 0.0f );
	out[ 3 ] = glm::simdVec4( 0.0f );
}

INLINE bool operator==
(
	glm::simdVec4 const & a,
	glm::simdVec4 const & b
)
{
	__m128 data = _mm_cmpeq_ps( a.Data, b.Data );

	__m64 hi, lo;
	_mm_storeh_pi( &hi, data );
	_mm_storel_pi( &lo, data );

	long long h64 = _m_to_int64( hi );
	long long l64 = _m_to_int64( lo );

	return ( ( l64 ^ 0xFFFFFFFFFFFFFFFF ) == 0 ) && ( ( h64 ^ 0xFFFFFFFFFFFFFFFF ) == 0 );
}

INLINE bool operator!=
(
	glm::simdVec4 const & a,
	glm::simdVec4 const & b
)
{
	return !( a == b );
}

#endif // GLM_ARCH != GLM_ARCH_PURE

namespace ext {

INLINE btVector3 to_bullet( const glm::vec3& v )
{
    return btVector3( v.x, v.y, v.z );
}

INLINE btMatrix3x3 to_bullet( const glm::mat3& m )
{
    btMatrix3x3 bm;
    bm[ 0 ] = to_bullet( m[ 0 ] );
    bm[ 1 ] = to_bullet( m[ 1 ] );
    bm[ 2 ] = to_bullet( m[ 2 ] );

    return bm;
}

INLINE btTransform to_bullet( const glm::mat4& m )
{
    btTransform t;
    t.setIdentity();
    t.setFromOpenGLMatrix( glm::value_ptr( m ) );
    return t;
}

INLINE glm::vec3 from_bullet( const btVector3& v )
{
    return glm::vec3( v.x(), v.y(), v.z() );
}

INLINE glm::mat4 from_bullet( const btTransform& t )
{
    glm::mat4 gm;

    float mat[ 16 ];
    memset( mat, 0, sizeof( mat ) );
    t.getOpenGLMatrix( mat );

    gm[ 0 ][ 0 ] = mat[ 0 ];
    gm[ 0 ][ 1 ] = mat[ 1 ];
    gm[ 0 ][ 2 ] = mat[ 2 ];
    gm[ 0 ][ 3 ] = mat[ 3 ];

    gm[ 1 ][ 0 ] = mat[ 4 ];
    gm[ 1 ][ 1 ] = mat[ 5 ];
    gm[ 1 ][ 2 ] = mat[ 6 ];
    gm[ 1 ][ 3 ] = mat[ 7 ];

    gm[ 2 ][ 0 ] = mat[ 8  ];
    gm[ 2 ][ 1 ] = mat[ 9  ];
    gm[ 2 ][ 2 ] = mat[ 10 ];
    gm[ 2 ][ 3 ] = mat[ 11 ];

    gm[ 3 ][ 0 ] = mat[ 12 ];
    gm[ 3 ][ 1 ] = mat[ 13 ];
    gm[ 3 ][ 2 ] = mat[ 14 ];
    gm[ 3 ][ 3 ] = mat[ 15 ];

    return gm;
}

INLINE glm::mat3 from_bullet( const btMatrix3x3& m )
{
    glm::mat3 gm;
    gm[ 0 ] = from_bullet( m[ 0 ] );
    gm[ 1 ] = from_bullet( m[ 1 ] );
    gm[ 2 ] = from_bullet( m[ 2 ] );
    return gm;
}

template < typename vec_type >
struct maxmin_pair
{
    vec_type max = vec_type( 0.0f );
    vec_type min = vec_type( 0.0f );
};

enum class cardinal_plane_normal
{
    x = 0, // -> plane normal n = < 1, 0, 0 >, or plane x = 0
    y = 1, // -> plane normal n = < 0, 1, 0 >, or plane y = 0
    z = 2  // -> plane normal n = < 0, 0, 1 >, or plane z = 0
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

INLINE bool bound_range_max( float x, float min, float max )
{
	return min <= x && x < max;
}

INLINE glm::mat3 project_cardinal( const glm::mat3& m, index_t normalAxis )
{
    glm::mat3 copy( m );
    copy[ normalAxis ][ 0 ] = 0.0f;
    copy[ normalAxis ][ 1 ] = 0.0f;
    copy[ normalAxis ][ 2 ] = 0.0f;

    return std::move( copy );
}

INLINE glm::vec3 project_cardinal( const glm::vec3& v, cardinal_plane_normal p )
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

INLINE cardinal_plane_normal best_cardinal_plane_normal( const glm::vec3& normal )
{
    glm::mat3 cardinalNormals( 1.0f );

    uint32_t index = 0;
    float max = 0.0f;

    for ( uint32_t i = 0; i < 3; ++i )
    {
        float d = glm::abs( glm::dot( normal, cardinalNormals[ i ] ) );

        if ( d > max )
        {
            max = d;
            index = i;
        }
    }

    return ( cardinal_plane_normal )( index );
}

template < typename entry_type >
using maxmin_fetch_fn_t = std::function< glm::vec3( const entry_type& e ) >;

template < typename list_type, typename entry_type >
INLINE vec3_maxmin_pair_t maxmin_from_list( const list_type& list, maxmin_fetch_fn_t< entry_type > fetchFn )
{
    vec3_maxmin_pair_t pair;

    glm::vec3 line( 0.0f );

    float maxLen = 0.0f;

    for ( const entry_type& a: list )
    {
        glm::vec3 p = fetchFn( a );

        for ( const entry_type& b: list )
        {
            glm::vec3 p0 = fetchFn( b );

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

INLINE glm::mat3 scale( const glm::mat3& m, const glm::vec3& size )
{
	glm::mat3 s( m );
	s[ 0 ] *= size[ 0 ];
	s[ 1 ] *= size[ 1 ];
	s[ 2 ] *= size[ 2 ];

	return std::move( s );
}

} // namespace ext

} // namespace glm

//
// STL specializations
//

namespace std {

template <> struct hash< glm::vec3 >
{
    size_t operator()( const glm::vec3& x ) const
    {
        size_t a = size_t( glm::length( x ) );

        return std::hash< size_t >()( a );
    }
};

template<> struct equal_to< glm::vec3 >
{
    bool operator()( const glm::vec3& lhs, const glm::vec3& rhs ) const
    {
        return lhs == rhs;
    }
};

template<> struct less< glm::vec3 >
{
    bool operator()( const glm::vec3& lhs, const glm::vec3& rhs ) const
    {
        return glm::all( glm::lessThan( lhs, rhs ) );
    }
};

}
