#include "geom.h"
#include <array>
#include <string.h>
#include <glm/gtx/projection.hpp>

namespace geom {

const float DISTANCE_THRESH = 0.03125f;

glm::vec3 PlaneProject( const glm::vec3& p, const glm::vec3& origin, const glm::vec3& normal )
{
	glm::vec3 originToP( p - origin );

	float dist = glm::dot( originToP, normal );
	return std::move( glm::vec3( p - normal * dist ) );
}

bool RayRayTest( const ray_t& r0, const ray_t& r1, float& t0, float& t1 )
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
	: extents( 1.0f ), origin( 0.0f ),
	  distance( 0.0f )
{
}

half_space_t::half_space_t( const glm::mat3& extents_, const glm::vec3& origin_, float distance_ )
	: extents( extents_ ),
	  origin( origin_ ),
	  distance( distance_ )
{
}

bool half_space_t::TestBounds( glm::vec3& normal, const glm::mat3& srcExtents, const glm::vec3& srcOrigin ) const
{
	UNUSEDPARAM( normal );

	float szLength( glm::length( extents[ 0 ] + extents[ 1 ] + extents[ 2 ] ) );

	glm::vec3 a( srcOrigin + srcExtents[ 0 ] );
	glm::vec3 b( srcOrigin + srcExtents[ 1 ] );
	glm::vec3 c( srcOrigin + srcExtents[ 2 ] );

	if ( glm::distance( origin, a ) > szLength ) return false;
	if ( glm::distance( origin, b ) > szLength ) return false;
	if ( glm::distance( origin, c ) > szLength ) return false;

	auto LIntersects = []( const glm::vec3& p0, const glm::vec3& d0, const glm::vec3& origin, const glm::mat3& extents ) -> bool
	{
		glm::vec3 a( origin + extents[ 0 ] ),
				  b( origin + extents[ 1 ] ),
				  c( origin + extents[ 2 ] );

		float ax( glm::dot( a - p0, d0 ) );
		float ay( glm::dot( b - p0, d0 ) );
		float az( glm::dot( c - p0, d0 ) );

		float dlen = glm::length( d0 );

		if ( d0 != extents[ 0 ] && ax > 0.0f && ax <= dlen ) return true;
		if ( d0 != extents[ 1 ] && ay > 0.0f && ay <= dlen ) return true;
		if ( d0 != extents[ 2 ] && az > 0.0f && az <= dlen ) return true;

		return false;
	};

	normal = /*extents[ 0 ] + extents[ 1 ] +*/ extents[ 2 ];

	if ( LIntersects( origin, extents[ 0 ], srcOrigin, srcExtents ) ) return true;
	if ( LIntersects( origin, extents[ 1 ], srcOrigin, srcExtents ) ) return true;
	if ( LIntersects( origin, extents[ 2 ], srcOrigin, srcExtents ) ) return true;

	if ( LIntersects( srcOrigin, srcExtents[ 0 ], origin, extents ) ) return true;
	if ( LIntersects( srcOrigin, srcExtents[ 1 ], origin, extents ) ) return true;
	if ( LIntersects( srcOrigin, srcExtents[ 2 ], origin, extents ) ) return true;

	return false;
}

void half_space_t::Draw( rend::imm_draw_t& drawer ) const
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
	: transform( transform_),
      color( glm::vec4( 1.0f ) )
{
}

bounding_box_t::bounding_box_t( bounding_box_t&& m )
	: transform( std::move( m.transform ) ),
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

		//glm::vec3 tedge( transform3 * edge );
		edges[ axis ] = std::move( edge );
		edgeCount++;

		if ( edgeCount == 3 )
		{
			break;
		}
	}
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

	static uint32_t count = 0;
	if ( halfSpace.TestBounds( normal, t, origin ) )
	{
		printf( "YEAH: %iu\n", ++count );
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

} // namespace geom
