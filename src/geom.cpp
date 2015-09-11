#include "debug.h"
#include "geom.h"
#include "bvh.h"
#include "stl_ext.hpp"
#include <array>
#include <string.h>
#include <glm/gtx/projection.hpp>
#include <glm/gtx/simd_vec4.hpp>
#include <glm/gtx/simd_mat4.hpp>

namespace geom {
#if GLM_ARCH != GLM_ARCH_PURE
    using vec_t = glm::simdVec4;
    using mat_t = glm::simdMat4;

    INLINE void Mat3ToSimd( mat_t& out, const glm::mat3& in )
	{
        out[ 0 ] = vec_t( in[ 0 ], 0.0f );
        out[ 1 ] = vec_t( in[ 1 ], 0.0f );
        out[ 2 ] = vec_t( in[ 2 ], 0.0f );
        out[ 3 ] = vec_t( 0.0f );
	}

	bool operator==
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

	bool operator!=
	(
		glm::simdVec4 const & a,
		glm::simdVec4 const & b
	)
	{
		return !( a == b );
	}

    template< size_t numVectors, size_t numMatrices >
    struct simd_conversion_t
    {
        std::array< vec_t, numVectors > vectors;
        std::array< mat_t, numMatrices > matrices;

        simd_conversion_t( const std::array< glm::vec3, numVectors >& v,
                           const std::array< glm::mat3, numMatrices >& m );
    };

    template< size_t numVectors, size_t numMatrices >
    simd_conversion_t< numVectors, numMatrices >::simd_conversion_t(
            const std::array< glm::vec3, numVectors >& v,
            const std::array< glm::mat3, numMatrices >& m )
    {
        for ( uint32_t i = 0; i < numVectors; ++i )
        {
            vectors[ i ] = std::move( vec_t( v[ i ], 1.0f ) );
        }

        for ( uint32_t i = 0; i < numMatrices; ++i )
        {
            Mat3ToSimd( matrices[ i ], m[ i ] );
        }
    }

    using simd_intersect_convert_t = simd_conversion_t< 2, 2 >;

#else
    using vec_t = glm::vec3;
    using mat_t = glm::mat3;
#endif // GLM_ARCH_PURE

    struct intersect_comp_t
    {
        struct sat_params_t
        {
            const vec_t& p0;
            const vec_t& d0;
            const vec_t& origin;
            const mat_t extents;
        };

        const vec_t& origin;
        const vec_t& srcOrigin;

        const mat_t& extents;
        const mat_t& srcExtents;

        float sizeLength;

        std::array< vec_t, 3 > sourcePoints;

        intersect_comp_t(
            const vec_t& origin_,
            const mat_t& extents_,
            const vec_t& srcOrigin_,
            const mat_t& srcExtents_ );

        bool ValidateDistance( void );

        bool TestIntersection( const vec_t& p0, const vec_t& d0, const vec_t& origin_, const mat_t& extents_ );
    };

    intersect_comp_t::intersect_comp_t(
        const vec_t& origin_,
        const mat_t& extents_,
        const vec_t& srcOrigin_,
        const mat_t& srcExtents_ )
        : origin( origin_ ),
          srcOrigin( srcOrigin_ ),
          extents( extents_ ),
          srcExtents( srcExtents_ ),
          sizeLength( glm::length( extents[ 0 ] + extents[ 1 ] + extents[ 2 ] ) ),
          sourcePoints( {{
            srcOrigin_ + srcExtents_[ 0 ],
            srcOrigin_ + srcExtents_[ 1 ],
            srcOrigin_ + srcExtents_[ 2 ]
          }} )

    {}

    bool intersect_comp_t::ValidateDistance( void )
    {
        if ( glm::distance( origin, sourcePoints[ 0 ] ) > sizeLength ) return false;
        if ( glm::distance( origin, sourcePoints[ 1 ] ) > sizeLength ) return false;
        if ( glm::distance( origin, sourcePoints[ 2 ] ) > sizeLength ) return false;

        return true;
    }

    bool intersect_comp_t::TestIntersection( const vec_t& p0, const vec_t& d0, const vec_t& origin_, const mat_t& extents_ )
    {
        vec_t a( origin_ + extents_[ 0 ] ),
              b( origin_ + extents_[ 1 ] ),
              c( origin_ + extents_[ 2 ] );

        vec_t ax( a - p0 );
        vec_t ay( b - p0 );
        vec_t az( c - p0 );

        vec_t dx( glm::proj( ax, d0 ) );
        vec_t dy( glm::proj( ay, d0 ) );
        vec_t dz( glm::proj( az, d0 ) );

        float dlen = glm::length( d0 );

		if ( glm::length( dx ) > dlen ) return false;
		if ( glm::length( dy ) > dlen ) return false;
		if ( glm::length( dz ) > dlen ) return false;

        return true;
    }
}

const float DISTANCE_THRESH = 0.03125f;

glm::vec3 plane_project( const glm::vec3& origin, const plane& p )
{
    glm::vec3 originToP( p.mPoint - origin );

    float dist = glm::dot( originToP, p.mNormal );

    return std::move( glm::vec3( origin + p.mNormal * dist ) );
}

bool test_ray_ray( const ray& r0, const ray& r1, float& t0, float& t1 )
{
	glm::vec3 crossDir( glm::cross( r0.d, r1.d ) );

	float dist = glm::distance( r0.p, r1.p );

	if ( crossDir == glm::zero< glm::vec3 >() && dist > DISTANCE_THRESH  )
	{
		return false;
	}

	glm::vec3 pointDiff( r0.p - r1.p );
	glm::vec3 pointDiffCross1( glm::cross( pointDiff, r1.d ) );

	float crossMag = glm::length( crossDir );
	float invMagSqr = 1.0f / ( crossMag * crossMag );

	t0 = glm::dot( pointDiffCross1, crossDir ) * invMagSqr;

	glm::vec3 pointDiffCross0( glm::cross( pointDiff, r0.d ) );
	t1 = glm::dot( pointDiffCross0, crossDir ) * invMagSqr;

	glm::vec3 pr0( r0.p + r0.d * t0 );
	glm::vec3 pr1( r1.p + r1.d * t1 );

	if ( glm::distance( pr0, pr1 ) > DISTANCE_THRESH )
	{
		// skew lines
		return false;
	}

	return true;
}

//-------------------------------------------------------------------------------------------------------
// half_space_t
//-------------------------------------------------------------------------------------------------------

halfspace::halfspace( void )
    : halfspace( glm::mat3( 1.0f ), glm::vec3( 0.0f ), 0.0f )
{
}

halfspace::halfspace( const glm::mat3& extents_, const glm::vec3& origin_, float distance_ )
    : bounds_primitive( BOUNDS_PRIM_HALFSPACE ),
      extents( extents_ ),
	  origin( origin_ ),
	  distance( distance_ )
{
}

halfspace::halfspace( const obb& bounds, const glm::vec3& normal )
    : halfspace( glm::mat3( 1.0f ), glm::vec3( 0.0f ), 0.0f )
{
    using corner_t = obb::corner_type;

    glm::vec3 upAxis( bounds[ 1 ] );
    glm::vec3 boundsOrigin( bounds[ 3 ] );
    glm::vec3 boundsSize( bounds.size() );

    // normalize the cross on extents[ 0 ] so that we don't scale more than is necessary
    extents[ 0 ] = std::move( glm::normalize( glm::cross( normal, upAxis ) ) ) * boundsSize[ 0 ];
    extents[ 1 ] = glm::normalize( upAxis ) * boundsSize[ 1 ];
    extents[ 2 ] = normal * 0.1f;

    // FIXME: scaling the normal by the boundsSize as a vector operation seems
    // a little odd. Maybe something like boundsOrigin + glm::normalize( normal )
    // would be better.
    glm::vec3 faceCenter( std::move( boundsOrigin + normal * boundsSize * 0.5f ) );

    // TODO: take into account ALL points
    std::array< corner_t, 4 > lowerPoints =
    {{
        obb::CORNER_MIN,
        obb::CORNER_NEAR_DOWN_RIGHT,
        obb::CORNER_FAR_DOWN_RIGHT,
        obb::CORNER_FAR_DOWN_LEFT
    }};

    for ( corner_t face: lowerPoints )
    {
        glm::vec3 point( bounds.corner( ( corner_t ) face ) );
        glm::vec3 pointToCenter( faceCenter - point );

        // Not in same plane; move on
        if ( triple_product( pointToCenter, extents[ 0 ], extents[ 1 ] ) != 0.0f )
        {
            continue;
        }

        // Half space axes will be outside of the bounds; move on
        if ( !bounds.range( point + extents[ 0 ], false ) || !bounds.range( point + extents[ 1 ], false ) )
        {
            continue;
        }

        origin = point;
        break;
    }

    distance = glm::dot( extents[ 2 ], origin );
}

halfspace::halfspace( const halfspace& c )
    : halfspace( c.extents, c.origin, c.distance )
{
}

halfspace& halfspace::operator=( halfspace c )
{
    if ( this != &c )
    {
        extents = c.extents;
        origin = c.origin;
        distance = c.distance;
    }

    return *this;
}

bool halfspace::test_bounds( glm::vec3& normal, const glm::mat3& srcExtents, const glm::vec3& srcOrigin ) const
{
#if GLM_ARCH == GLM_ARCH_PURE
    geom::intersect_comp_t test( origin, extents, srcOrigin, srcExtents );
    normal = std::move( extents[ 2 ] );

#else
    geom::simd_intersect_convert_t c( {{ origin, srcOrigin }}, {{ extents, srcExtents }} );
    geom::intersect_comp_t test( c.vectors[ 0 ], c.matrices[ 0 ], c.vectors[ 1 ], c.matrices[ 1 ] );

    normal = std::move( glm::vec3( glm::vec4_cast( c.matrices[ 0 ][ 2 ] ) ) );
#endif // GLM_ARCH_PURE

    if ( !test.ValidateDistance() )
    {
        return false;
    }

	if ( test.TestIntersection( test.origin, test.extents[ 0 ], test.srcOrigin, test.srcExtents ) ) return true;
    if ( test.TestIntersection( test.origin, test.extents[ 1 ], test.srcOrigin, test.srcExtents ) ) return true;
    if ( test.TestIntersection( test.origin, test.extents[ 2 ], test.srcOrigin, test.srcExtents ) ) return true;

    if ( test.TestIntersection( test.srcOrigin, test.srcExtents[ 0 ], test.origin, test.extents ) ) return true;
    if ( test.TestIntersection( test.srcOrigin, test.srcExtents[ 0 ], test.origin, test.extents ) ) return true;
    if ( test.TestIntersection( test.srcOrigin, test.srcExtents[ 0 ], test.origin, test.extents ) ) return true;

	return false;
}

void halfspace::draw( imm_draw& drawer ) const
{
    drawer.begin( GL_LINES );

    drawer.vertex( origin );
    drawer.vertex( origin + extents[ 0 ] );

    drawer.vertex( origin );
    drawer.vertex( origin + extents[ 1 ] );

    drawer.vertex( origin );
    drawer.vertex( origin + extents[ 2 ] * 2.0f );

    drawer.end();
}

//-------------------------------------------------------------------------------------------------------
// bounding_box_t
//-------------------------------------------------------------------------------------------------------

obb::obb( const glm::mat4& transform_ )
    : bounds_primitive( BOUNDS_PRIM_BOX ),
      mAxes( transform_),
      mColor( glm::vec4( 1.0f ) )
{
}

obb::obb( obb&& m )
    : bounds_primitive( m ),
      mAxes( std::move( m.mAxes ) ),
      mColor( std::move( m.mColor ) )
{
}

obb::obb( const obb& c )
    : bounds_primitive( c ),
      mAxes( c.mAxes ),
      mColor( c.mColor )
{}

glm::vec3 obb::center( void ) const
{
   return glm::vec3( mAxes[ 3 ] );
}

glm::vec3 obb::size( void ) const
{
    return corner( CORNER_MAX ) - corner( CORNER_MIN );
}

glm::vec3 obb::radius( void ) const
{
    return corner( CORNER_MAX ) - glm::vec3( mAxes[ 3 ] );
}

glm::vec3 obb::corner( corner_type index ) const
{
    return glm::vec3( mAxes * glm::vec4( corner_identity( index ), 1.0f ) );
}

// Iterate through the identity
// corners and compute a direction
// from our origin point ( denoted by index ) to
// the corner of the current iteration. If
// we deem the direction to be an edge,
// transform the edge by the bounds
// transformation and map
// it to the respective right, up, and forward
// axes of the returned matrix.
void obb::edges_from_corner( corner_type index, glm::mat3& edges ) const
{
    glm::vec3 origin( corner( index ) );

    glm::mat3 transform3( mAxes );

	int32_t edgeCount = 0;

	for ( int32_t i = 0; i < 7; ++i )
	{
        if ( ( corner_type )i == index )
		{
			continue;
		}

        glm::vec3 edge( corner( ( corner_type ) i ) - origin );

		float edgeLen = glm::length( edge );

		float dx = glm::abs( glm::dot( edge, transform3[ 0 ] ) );
		float dy = glm::abs( glm::dot( edge, transform3[ 1 ] ) );
		float dz = glm::abs( glm::dot( edge, transform3[ 2 ] ) );

		// if any of the dot products are 1, then the rest are 0.
		// If none of them are one, then we don't have an edge
		// since the candidate we just computed isn't parallel
		// to a cardinal axis.
		if ( dx != edgeLen && dy != edgeLen && dz != edgeLen )
		{
			continue;
		}

		int32_t axis;

		// Check paralellity with each axis, map the cardinality to the appropriate axial slot
		if ( dx == edgeLen )
		{
			axis = 0;
		}
		else if ( dy == edgeLen )
		{
			axis = 1;
		}
		else
		{
			axis = 2;
		}

		edges[ axis ] = std::move( edge );
		edgeCount++;

		if ( edgeCount == 3 )
		{
			break;
		}
	}
}

bool obb::encloses( const obb& box ) const
{
#if GLM_ARCH == GLM_ARCH_PURE

    // intersect_comp_t takes mostly references
    // in its ctor, so we just make some copies to maintain
    // scope :D
    glm::vec3 va( transform[ 3 ] );
    glm::mat3 ma( transform );

    glm::vec3 vb( box.transform[ 3 ] );
    glm::mat3 mb( box.transform );

    geom::intersect_comp_t test( va, ma, vb, mb );

#else
    geom::simd_intersect_convert_t c( {{  glm::vec3( mAxes[ 3 ] ), glm::vec3( box.mAxes[ 3 ] ) }},
    {{  glm::mat3( mAxes ), glm::mat3( box.mAxes ) }} );

    geom::intersect_comp_t test( c.vectors[ 0 ], c.matrices[ 0 ], c.vectors[ 1 ], c.matrices[ 1 ] );
#endif

    if ( !test.ValidateDistance() )
    {
        return false;
    }

    if ( !test.TestIntersection( test.origin, test.extents[ 0 ], test.srcOrigin, test.srcExtents ) ) return false;
    if ( !test.TestIntersection( test.origin, test.extents[ 1 ], test.srcOrigin, test.srcExtents ) ) return false;
    if ( !test.TestIntersection( test.origin, test.extents[ 2 ], test.srcOrigin, test.srcExtents ) ) return false;

    geom::mat_t negAxis( test.srcExtents );

    if ( !test.TestIntersection( test.origin, test.extents[ 0 ], test.srcOrigin, -negAxis ) ) return false;
    if ( !test.TestIntersection( test.origin, test.extents[ 1 ], test.srcOrigin, -negAxis ) ) return false;
    if ( !test.TestIntersection( test.origin, test.extents[ 2 ], test.srcOrigin, -negAxis ) ) return false;

    return true;
}

bool obb::range( glm::vec3 v, bool inversed ) const
{
    if ( !inversed )
    {
        v = std::move( inv_linear_axes() * v );
    }

    obb::maxmin_pair3D_t mm = std::move( maxmin( true ) );

    return glm::all( glm::lessThanEqual( mm.min, v ) )
        && glm::all( glm::lessThanEqual( v, mm.max ) );
}

using cardinal_plane = glm::ext::cardinal_plane;

namespace {

    // returns true if a intersects b
    bool pointset_intersects( cardinal_plane planeType,
                              const obb::pointset3D_t& a,
                              const obb::pointset3D_t& b )
    {
        obb::pointset3D_t cardinalB;
        for ( const auto& p: b )
        {
            cardinalB.insert( glm::ext::project_cardinal( p, planeType ) );
        }

        uint32_t axis0 = ( ( uint32_t )planeType + 1 ) % 3;
        uint32_t axis1 = ( ( uint32_t )planeType + 2 ) % 3;

        glm::ext::vec3_maxmin_pair_t mm = glm::ext::maxmin_from_list( cardinalB );

        for ( auto i = a.begin(); i != a.end(); ++i )
        {
            const glm::vec3 u( glm::ext::project_cardinal( *i, planeType ) );

            if ( glm::ext::range( u, mm.min, mm.max, axis0 )
                 && glm::ext::range( u, mm.min, mm.max, axis1 ) )
            {
                return true;
            }
        }

        return false;
    }

    bool face_intersects( obb::face_type face,
                          const obb& source,
                          const obb::pointlist3D_t& a,
                          const obb::pointlist3D_t& b )
    {
        plane facePlane;
        source.face_plane( face, facePlane );

        obb::pointset3D_t a0 = std::move( source.face_project( facePlane, a ) );
        obb::pointset3D_t b0 = std::move( source.face_project( facePlane, b ) );

        cardinal_plane cp = glm::ext::best_cardinal_plane( facePlane.mNormal );

        return pointset_intersects( cp, a0, b0 );
    }
}

bool obb::intersects( glm::vec3& normal, const obb& bounds ) const
{
    obb boundsCopy( bounds );

    glm::mat4 t( bounds.linear_axes() * bounds.inv_linear_axes() );
    t[ 3 ] = glm::vec4( bounds.center(), 1.0f );

    boundsCopy.axes( t );

    obb thisCopy( *this );
    thisCopy.orientation( bounds.inv_linear_axes() * thisCopy.linear_axes() );

    pointlist3D_t thisPoints, otherPoints;
    thisCopy.points( thisPoints );
    boundsCopy.points( otherPoints );

    for ( int32_t i = 0; i < 6; ++i )
    {
        if ( !face_intersects( ( face_type )i, boundsCopy, thisPoints, otherPoints ) )
        {
            return false;
        }
    }

    normal = glm::normalize( center() - bounds.center() );

    return true;
}

bool obb::intersects( glm::vec3& normal, const halfspace& halfSpace ) const
{
    glm::mat3 t( mAxes );
    glm::vec3 origin( mAxes[ 3 ] );

    if ( halfSpace.test_bounds( normal, t, origin ) )
	{
		return true;
	}

	return false;
}

// Find the closest 3 faces
// Compute intersections;
// then make sure the ray will be within the bounds of the three faces;
bool obb::ray_intersection( ray& r, bool earlyOut ) const
{
    glm::mat3 i( inv_linear_axes() );
    ray tmp( i * r.p, i * r.d, r.t );
    r = tmp;

    bool inside = range( r.calc_position(), true );

	if ( earlyOut && inside )
	{
		r.t = 1.0f;
		return true;
	}

    obb::maxmin_pair3D_t mm = std::move( maxmin( true ) );

	glm::vec3 maxT( 0.0f );
	for ( int32_t i = 0; i < 3; ++i )
	{
        if ( r.d[ i ] >= 0.0f )
		{
			maxT[ i ] = ( mm.min[ i ] - r.p[ i ] ) / r.d[ i ];
		}
		else
		{
			maxT[ i ] = ( mm.max[ i ] - r.p[ i ] ) / r.d[ i ];
		}
	}

    int32_t mi = glm::ext::max_index( maxT );
	assert( mi != -1 );

	if ( 0.0f <= mi && mi <= glm::length( r.calc_position() ) )
	{
		r.t = maxT[ mi ];
        glm::vec3 rp( r.calc_position() );

        return range( rp, true );

        int32_t i0 = ( mi + 1 ) % 3;
        int32_t i1 = ( mi + 2 ) % 3;

        return glm::ext::range( rp, mm.min, mm.max, i0 )
            && glm::ext::range( rp, mm.min, mm.max, i1 );
	}

	return false;
}

obb::maxmin_pair3D_t obb::maxmin( bool inverse ) const
{
    obb::maxmin_pair3D_t mm;

    mm.max = corner( CORNER_MAX );
    mm.min = corner( CORNER_MIN );

    if ( inverse )
    {
        glm::mat3 i( inv_linear_axes() );

        mm.max = i * mm.max;
        mm.min = i * mm.min;
    }

	return std::move( mm );
}

obb::pointset3D_t obb::face_project( const plane& facePlane, const pointlist3D_t& sourcePoints ) const
{
    pointset3D_t projected;

    for ( const auto& p: sourcePoints )
    {
        projected.insert( plane_project( p, facePlane ) );
    }

    return std::move( projected );
}

void obb::face_plane( face_type face, plane& thePlane ) const
{
    glm::vec3 p;

    switch ( face )
    {
        case obb::FACE_TOP:
            p = corner( CORNER_MAX );
            thePlane.mNormal = glm::vec3( 0.0f, 1.0f, 0.0f );
            break;
        case obb::FACE_RIGHT:
            p = corner( CORNER_MAX );
            thePlane.mNormal = glm::vec3( 1.0f, 0.0f, 0.0f );
            break;
        case obb::FACE_FRONT:
            p = corner( CORNER_NEAR_UP_RIGHT );
            thePlane.mNormal = glm::vec3( 0.0f, 0.0f, 1.0f );
            break;
        case obb::FACE_LEFT:
            p = corner( CORNER_NEAR_UP_LEFT );
            thePlane.mNormal = glm::vec3( -1.0f, 0.0f, 0.0f );
            break;
        case obb::FACE_BACK:
            p = corner( CORNER_FAR_UP_LEFT );
            thePlane.mNormal = glm::vec3( 0.0f, 0.0f, -1.0f );
            break;
        case obb::FACE_BOTTOM:
            p = corner( CORNER_NEAR_DOWN_RIGHT );
            thePlane.mNormal = glm::vec3( 0.0f, -1.0f, 0.0f );
            break;
    }

    thePlane.mNormal = glm::normalize( linear_axes() * thePlane.mNormal );
    thePlane.mPoint = p;
    thePlane.mDistance = glm::dot( p, thePlane.mNormal );
}

void obb::color( const glm::vec4& color_ )
{
    mColor = color_;
}
