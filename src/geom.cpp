#include "geom.h"
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

        if ( glm::abs( glm::length( dx ) ) > dlen ) return false;
        if ( glm::abs( glm::length( dy ) ) > dlen ) return false;
        if ( glm::abs( glm::length( dz ) ) > dlen ) return false;

        return true;
    }
}

const float DISTANCE_THRESH = 0.03125f;

glm::vec3 G_PlaneProject( const glm::vec3& p, const glm::vec3& origin, const glm::vec3& normal )
{
	glm::vec3 originToP( p - origin );

	float dist = glm::dot( originToP, normal );
	return std::move( glm::vec3( p - normal * dist ) );
}

bool G_RayRayTest( const ray_t& r0, const ray_t& r1, float& t0, float& t1 )
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

half_space_t::half_space_t( void )
    : half_space_t( glm::mat3( 1.0f ), glm::vec3( 0.0f ), 0.0f )
{
}

half_space_t::half_space_t( const glm::mat3& extents_, const glm::vec3& origin_, float distance_ )
    : bounds_primitive_t( BOUNDS_PRIM_HALFSPACE ),
      extents( extents_ ),
	  origin( origin_ ),
	  distance( distance_ )
{
}

half_space_t::half_space_t( const bounding_box_t& bounds, const glm::vec3& normal )
    : half_space_t( glm::mat3( 1.0f ), glm::vec3( 0.0f ), 0.0f )
{
    using corner_t = bounding_box_t::corner_t;

    glm::vec3 upAxis( bounds[ 1 ] );
    glm::vec3 boundsOrigin( bounds[ 3 ] );
    glm::vec3 boundsSize( bounds.GetSize() );

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
        bounding_box_t::CORNER_MIN,
        bounding_box_t::CORNER_NEAR_DOWN_RIGHT,
        bounding_box_t::CORNER_FAR_DOWN_RIGHT,
        bounding_box_t::CORNER_FAR_DOWN_LEFT
    }};

    for ( corner_t face: lowerPoints )
    {
        glm::vec3 point( bounds.GetCorner( ( corner_t ) face ) );
        glm::vec3 pointToCenter( faceCenter - point );

        // Not in same plane; move on
        if ( G_TripleProduct( pointToCenter, extents[ 0 ], extents[ 1 ] ) != 0.0f )
        {
            continue;
        }

        // Half space axes will be outside of the bounds; move on
        if ( !bounds.EnclosesPoint( point + extents[ 0 ] ) || !bounds.EnclosesPoint( point + extents[ 1 ] ) )
        {
            continue;
        }

        origin = point;
        break;
    }

    distance = glm::dot( extents[ 2 ], origin );
}

half_space_t::half_space_t( const half_space_t& c )
    : half_space_t( c.extents, c.origin, c.distance )
{
}

half_space_t& half_space_t::operator=( half_space_t c )
{
    if ( this != &c )
    {
        extents = c.extents;
        origin = c.origin;
        distance = c.distance;
    }

    return *this;
}

bool half_space_t::TestBounds( glm::vec3& normal, const glm::mat3& srcExtents, const glm::vec3& srcOrigin ) const
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

void half_space_t::Draw( imm_draw_t& drawer ) const
{
	drawer.Begin( GL_LINES );

	drawer.Vertex( origin );
	drawer.Vertex( origin + extents[ 0 ] );

	drawer.Vertex( origin );
	drawer.Vertex( origin + extents[ 1 ] );

	drawer.Vertex( origin );
	drawer.Vertex( origin + extents[ 2 ] * 2.0f );

	drawer.End();
}

//-------------------------------------------------------------------------------------------------------
// bounding_box_t
//-------------------------------------------------------------------------------------------------------

bounding_box_t::bounding_box_t( const glm::mat4& transform_ )
    : bounds_primitive_t( BOUNDS_PRIM_BOX ),
      transform( transform_),
      color( glm::vec4( 1.0f ) )
{
}

bounding_box_t::bounding_box_t( bounding_box_t&& m )
    : bounds_primitive_t( BOUNDS_PRIM_BOX ),
      transform( std::move( m.transform ) ),
	  color( std::move( m.color ) )
{
}

glm::vec3 bounding_box_t::GetCenter( void ) const
{
   return glm::vec3( transform[ 3 ] );
}

glm::vec3 bounding_box_t::GetSize( void ) const
{
	return GetCorner( CORNER_MAX ) - GetCorner( CORNER_MIN );
}

glm::vec3 bounding_box_t::GetRadius( void ) const
{
	return GetCorner( CORNER_MAX ) - glm::vec3( transform[ 3 ] );
}

glm::vec3 bounding_box_t::GetCorner( corner_t index ) const
{
	return glm::vec3( transform * glm::vec4( GetCornerIdentity( index ), 1.0f ) );
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
void bounding_box_t::GetEdgesFromCorner( corner_t index, glm::mat3& edges ) const
{
	glm::vec3 origin( GetCorner( index ) );

	glm::mat3 transform3( transform );

	int32_t edgeCount = 0;

	for ( int32_t i = 0; i < 7; ++i )
	{
		if ( ( corner_t )i == index )
		{
			continue;
		}

		glm::vec3 edge( GetCorner( ( corner_t ) i ) - origin );

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

bool bounding_box_t::Encloses( const bounding_box_t& box ) const
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
    geom::simd_intersect_convert_t c( {{  glm::vec3( transform[ 3 ] ), glm::vec3( box.transform[ 3 ] ) }},
    {{  glm::mat3( transform ), glm::mat3( box.transform ) }} );

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

bool bounding_box_t::IntersectsBounds( glm::vec3& normal, const bounding_box_t& bounds ) const
{
    half_space_t hs( glm::mat3( bounds.transform ), glm::vec3( bounds.transform[ 3 ] ), 0.0f );

	return hs.TestBounds( normal, glm::mat3( transform ), glm::vec3( transform[ 3 ] ) );
}

bool bounding_box_t::IntersectsHalfSpace( glm::vec3& normal, const half_space_t& halfSpace ) const
{
	glm::mat3 t( transform );
	glm::vec3 origin( transform[ 3 ] );

	if ( halfSpace.TestBounds( normal, t, origin ) )
	{
		return true;
	}

	return false;
}

// Find the closest 3 faces
// Compute intersections;
// then make sure the ray will be within the bounds of the three faces;
bool bounding_box_t::CalcIntersection( float& t0, const glm::vec3& ray, const glm::vec3& origin ) const
{
    // Quick early out; 0 implies no scaling necessary
	if ( EnclosesPoint( origin ) )
    {
		t0 = 0.0f;
		return true;
    }

    //std::array< plane_t, 3 > faces;
    std::array< float, 3 > intersections;

    int32_t fcount = 0;
    for ( int32_t i = 0; i < 6; ++i )
    {
        if ( fcount == 3 )
        {
            break;
        }

        plane_t p;

        GetFacePlane( ( face_t )i, p );

        float fx = p.normal.x * ray.x;
        float fy = p.normal.y * ray.y;
        float fz = p.normal.z * ray.z;

        float thedot = fx + fy + fz;

        // if true then face faces away from ray, so move on
        if ( thedot >= 0.0f )
        {
            continue;
        }

        float t = -( glm::dot( origin, p.normal ) - p.d ) / thedot;

        if ( isinf( t ) )
        {
            continue;
        }

        glm::vec3 r( origin + ray * t );

        // only one component can be nonzero, so we test
        // against our current face to ensure that we're not outside of the bounds
        // of the face

        // If the origin is in the range of the corresponding
        // face's axis, this implies that we're wasting our time: the origin
        // isn't actually inside of the bounds, and since we've ommitted
        // all normals which are within 90 degrees or less of the ray,
        // the only way this face can be hit is if we negate the ray, which we don't want.

        // front or back face
        if ( fz != 0.0f && !isinf( fz ) )
        {
            if ( !InXRange( r ) ) continue;
            if ( !InYRange( r ) ) continue;
        }
        // top or bottom face
        else if ( fy != 0.0f && !isinf( fy ) )
        {
            if ( !InZRange( r ) ) continue;
            if ( !InXRange( r ) ) continue;
        }
        // left or right face
        else if ( fx != 0.0f && !isinf( fx ) )
        {
            if ( !InZRange( r ) ) continue;
            if ( !InYRange( r ) ) continue;
        }
        else
        {
            continue;
        }

        intersections[ fcount++ ] = t;
    }

    // find closest intersection
	t0 = FLT_MAX;

    for ( int32_t i = 0; i < fcount; ++i )
    {
        if ( intersections[ i ] < t0 )
        {
            t0 = intersections[ i ];
        }
    }

	return t0 < FLT_MAX;
}

void bounding_box_t::GetFacePlane( face_t face, plane_t& plane ) const
{
    glm::vec3 p;

    switch ( face )
    {
        case bounding_box_t::FACE_TOP:
			p = GetCorner( CORNER_MAX );
            plane.normal = glm::vec3( 0.0f, 1.0f, 0.0f );
            break;
        case bounding_box_t::FACE_RIGHT:
			p = GetCorner( CORNER_MAX );
            plane.normal = glm::vec3( 1.0f, 0.0f, 0.0f );
            break;
        case bounding_box_t::FACE_FRONT:
			p = GetCorner( CORNER_NEAR_UP_RIGHT );
            plane.normal = glm::vec3( 0.0f, 0.0f, 1.0f );
            break;
        case bounding_box_t::FACE_LEFT:
			p = GetCorner( CORNER_NEAR_UP_LEFT );
            plane.normal = glm::vec3( -1.0f, 0.0f, 0.0f );
            break;
        case bounding_box_t::FACE_BACK:
			p = GetCorner( CORNER_FAR_UP_LEFT );
            plane.normal = glm::vec3( 0.0f, 0.0f, -1.0f );
            break;
        case bounding_box_t::FACE_BOTTOM:
			p = GetCorner( CORNER_NEAR_DOWN_RIGHT );
            plane.normal = glm::vec3( 0.0f, -1.0f, 0.0f );
            break;
    }

	plane.normal = glm::normalize( glm::mat3( transform ) * plane.normal );

    plane.d = glm::dot( p, plane.normal );
}

void bounding_box_t::SetDrawable( const glm::vec4& color_ )
{
    color = color_;
}

