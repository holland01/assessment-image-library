#include "geom.h"
#include <array>
#include <string.h>

namespace geom {

/*
namespace {
	struct corner_map_t
	{
		bounding_box_t::corner_t src;
		bounding_box_t::corner_t a, b, c;
	};

	std::array< corner_map_t, 8 > edgeTable =
	{{
		{ bounding_box_t::CORNER_MIN, bounding_box_t::CORNER_NEAR_DOWN_RIGHT, bounding_box_t::CORNER_NEAR_UP_LEFT, bounding_box_t::CORNER_FAR_DOWN_LEFT },
		{ bounding_box_t::CORNER_NEAR_DOWN_RIGHT, bounding_box_t::CORNER_MIN, bounding_box_t::CORNER_NEAR_UP_RIGHT, bounding_box_t::CORNER_FAR_DOWN_RIGHT },
		{ bounding_box_t::CORNER_NEAR_UP_LEFT, bounding_box_t::CORNER_MIN, bounding_box_t::CORNER_NEAR_UP_RIGHT, bounding_box_t::CORNER_FAR_UP_LEFT },
		{ bounding_box_t::CORNER_NEAR_UP_RIGHT, bounding_box_t::CORNER_NEAR_UP_LEFT, bounding_box_t::CORNER_NEAR_DOWN_RIGHT, bounding_box_t::CORNER_MAX },

		{ bounding_box_t::CORNER_FAR_DOWN_LEFT, bounding_box_t::CORNER_MIN, bounding_box_t::CORNER_FAR_UP_LEFT, bounding_box_t::CORNER_FAR_DOWN_RIGHT },
		{ bounding_box_t::CORNER_FAR_DOWN_RIGHT, bounding_box_t::CORNER_MAX, bounding_box_t::CORNER_FAR_DOWN_LEFT, bounding_box_t::CORNER_NEAR_DOWN_RIGHT },
		{ bounding_box_t::CORNER_FAR_UP_LEFT, bounding_box_t::CORNER_ }
	}};
}
*/

//-------------------------------------------------------------------------------------------------------
// bounding_box_t
//-------------------------------------------------------------------------------------------------------

bounding_box_t::bounding_box_t( void )
{
    Empty();
}

bounding_box_t::bounding_box_t( const glm::vec3& max, const glm::vec3& min, const glm::mat4& transform_, bool oriented_ )
	: transform( transform_),
	  oriented( oriented_ ),
	  maxPoint( transform_ * glm::vec4( max, 1.0f ) ),
	  minPoint( transform_ * glm::vec4( min, 1.0f ) ),
      color( glm::vec4( 1.0f ) )
{
}

bounding_box_t::bounding_box_t( bounding_box_t&& m )
	: transform( std::move( m.transform ) ),
	  oriented( m.oriented ),
	  maxPoint( std::move( m.maxPoint ) ),
	  minPoint( std::move( m.minPoint ) ),
	  color( std::move( m.color ) )
{
}

void bounding_box_t::Add( const glm::vec3& p )
{
    if ( p.x < minPoint.x ) minPoint.x = p.x;
    if ( p.y < minPoint.y ) minPoint.y = p.y;
    if ( p.z < minPoint.z ) minPoint.z = p.z;

    if ( p.x > maxPoint.x ) maxPoint.x = p.x;
    if ( p.y > maxPoint.y ) maxPoint.y = p.x;
    if ( p.z > maxPoint.z ) maxPoint.z = p.z;
}

void bounding_box_t::Empty( void )
{
    const float pseudoInfinity = 1e37f;

#ifdef AABB_MAX_Z_LESS_THAN_MIN_Z
    maxPoint = glm::vec3( -pseudoInfinity, -pseudoInfinity, pseudoInfinity );
    minPoint = glm::vec3( pseudoInfinity, pseudoInfinity, -pseudoInfinity );
#else
    maxPoint = glm::vec3( -pseudoInfinity );
    minPoint = glm::vec3( pseudoInfinity );
#endif
}

void bounding_box_t::TransformTo( const bounding_box_t& box, const glm::mat4& transform )
{
    maxPoint = minPoint = glm::vec3( transform[ 3 ] );

    for ( int32_t i = 0; i < 3; ++i )
    {
        float px = transform[ i ][ 0 ];
        float py = transform[ i ][ 1 ];
        float pz = transform[ i ][ 2 ];

        // Scale each basis' respective coordinate
        // along its respective axis for our max/min coordinates.
        // If pn (where 'n' is { x | y | z }) is > 0, compute box's point
        // in parallel with this one's. Otherwise, compute this instances
        // points opposite to that of the box's, in an effort to 'shrink'
        // our current points.

        if ( px > 0.0f )
        {
            minPoint.x += box.minPoint.x * px;
            maxPoint.x += box.maxPoint.x * px;
        }
        else
        {
            minPoint.x += box.maxPoint.x * px;
            maxPoint.x += box.minPoint.x * px;
        }

        if ( py > 0.0f )
        {
            minPoint.y += box.minPoint.y * py;
            maxPoint.y += box.maxPoint.y * py;
        }
        else
        {
            minPoint.y += box.maxPoint.y * py;
            maxPoint.y += box.minPoint.y * py;
        }

        if ( pz > 0.0f )
        {
            minPoint.z += box.minPoint.z * pz;
            maxPoint.z += box.maxPoint.z * pz;
        }
        else
        {
            minPoint.z += box.maxPoint.z * pz;
            maxPoint.z += box.minPoint.z * pz;
        }
    }
}

glm::vec3 bounding_box_t::GetMaxRelativeToNormal( const glm::vec3 &normal ) const
{
    glm::vec3 p( minPoint.x, minPoint.y, minPoint.z );

    if ( normal.x >= 0 )
        p.x = maxPoint.x;

    if ( normal.y >= 0 )
        p.y = maxPoint.y;

    if ( normal.z >= 0 )
        p.z = maxPoint.z;

    return p;
}

glm::vec3 bounding_box_t::GetMinRelativeToNormal( const glm::vec3 &normal ) const
{
    glm::vec3 n( maxPoint.x, maxPoint.y, maxPoint.z );

    if ( normal.x >= 0 )
        n.x = minPoint.x;

    if ( normal.y >= 0 )
        n.y = minPoint.y;

    if ( normal.z >= 0 )
        n.z = minPoint.z;

    return n;
}


glm::vec3 bounding_box_t::GetCenter( void ) const
{
    const glm::vec3& p = ( maxPoint + minPoint );

    return p / 2.0f;
}

glm::vec3 bounding_box_t::GetSize( void ) const
{
    return maxPoint - minPoint;
}

glm::vec3 bounding_box_t::GetRadius( void ) const
{
	return maxPoint - GetCenter();
}

glm::vec3 bounding_box_t::GetCorner( corner_t index ) const
{
	if ( oriented )
	{
		return glm::vec3( transform * glm::vec4( GetCornerIdentity( index ), 1.0f ) );
	}

	return glm::vec3(
		( ( int32_t ) index & 1 ) ? maxPoint.x : minPoint.x,
		( ( int32_t ) index & 2 ) ? maxPoint.y : minPoint.y,
		( ( int32_t ) index & 4 ) ? maxPoint.z : minPoint.z
	);
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
	glm::vec3 origin( GetCornerIdentity( index ) );

	glm::mat3 axes( 1.0f );
	glm::mat3 transform3( transform );

	int32_t edgeCount = 0;

	for ( int32_t i = 0; i < 7; ++i )
	{
		if ( ( corner_t )i == index )
		{
			continue;
		}

		glm::vec3 edge( GetCornerIdentity( ( corner_t ) i ) - origin );

		float dx = glm::abs( glm::dot( edge, axes[ 0 ] ) );
		float dy = glm::abs( glm::dot( edge, axes[ 1 ] ) );
		float dz = glm::abs( glm::dot( edge, axes[ 2 ] ) );

		// if any of the dot products are 1, then the rest are 0.
		// If none of them are one, then we don't have an edge
		// since the candidate we just computed isn't parallel
		// to a cardinal axis.
		if ( dx != 1.0f && dy != 1.0f && dz != 1.0f )
		{
			continue;
		}

		int32_t axis;

		// Check paralellity with each axis, map the cardinality to the appropriate axial slot
		if ( dx == 1.0f )
		{
			axis = 0;
		}
		else if ( dy == 1.0f )
		{
			axis = 1;
		}
		else // ( dz == 1.0f )
		{
			axis = 2;
		}

		glm::vec3 tedge( transform3 * edge );
		edges[ axis ] = std::move( tedge );
		edgeCount++;

		if ( edgeCount == 3 )
		{
			break;
		}
	}
}

bool bounding_box_t::IsEmpty( void ) const
{
    // Check to see if any of our
    // axes are inverted
    return ( maxPoint.x < minPoint.x )
        || ( maxPoint.y < minPoint.y )
#ifdef AABB_MAX_Z_LESS_THAN_MIN_Z
        || ( maxPoint.z < minPoint.z );
#else
        || ( maxPoint.z > minPoint.z );
#endif
}

bool bounding_box_t::InPointRange( float k ) const
{
    return ( maxPoint.x >= k && maxPoint.y >= k && maxPoint.z >= k )
        && ( minPoint.x <= k && minPoint.y <= k && minPoint.z <= k );
}


// test for a scalar triple product between the direction
// from the translation origin to the point to test; if it's not 0 (or within an arbitrary range, if desired), then the point
// isn't in the same plane and therefore fails.
// Otherwise, take the direction from the right axis of the half-space to the point.
// Then, negate the right axis and dot it with the direction just computed. If the result
// is less than zero, point is not within the half-space - test fails. Perform
// the same process with the point against the up axis.
// If these 3 tests pass, we has a winrar.
bool bounding_box_t::IntersectsHalfSpace( const half_space_t& halfSpace ) const
{
	std::array< glm::vec3, 8 > points;
	GetPoints( points );

	for ( const glm::vec3& p: points )
	{
		glm::vec3 originToP( p - halfSpace.origin );

		float d = TripleProduct( originToP, halfSpace.extents[ 1 ], halfSpace.extents[ 0 ] );

		// We give ourselves some wiggle room; if we're less than 1.5 then we just project the point onto the half-space plane.
		if ( glm::abs( d ) > 0.005f )
		{
			continue;
		}

		float dist = glm::dot( originToP, halfSpace.extents[ 2 ] );
		glm::vec3 projP( p - halfSpace.extents[ 2 ] * dist );

		glm::vec3 rightToP( projP - halfSpace.extents[ 0 ] );
		if ( glm::dot( rightToP, -halfSpace.extents[ 0 ] ) < 0.0f )
		{
			continue;
		}

		glm::vec3 upToP( projP - halfSpace.extents[ 1 ] );
		if ( glm::dot( upToP, -halfSpace.extents[ 1 ] ) < 0.0f )
		{
			continue;
		}

		// winrar
		return true;
	}

	return false;
}

// Find the closest 3 faces
// Compute intersections;
// then make sure the ray will be within the bounds of the three faces;
float bounding_box_t::CalcIntersection( const glm::vec3& ray, const glm::vec3& origin ) const
{
    // Quick early out; 0 implies no scaling necessary
	if ( EnclosesPoint( origin ) )
    {
        return 0.0f;
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

        //faces[ fcount ] = std::move( p );
        intersections[ fcount++ ] = t;

        //fcount++;
    }

    // find closest intersection
    float t0 = FLT_MAX;

    for ( int32_t i = 0; i < fcount; ++i )
    {
        if ( intersections[ i ] < t0 )
        {
            t0 = intersections[ i ];
        }
    }

    return t0;
}

void bounding_box_t::GetFacePlane( face_t face, plane_t& plane ) const
{
    glm::vec3 p;

    switch ( face )
    {
		case bounding_box_t::FACE_TOP:
			p = maxPoint;
            plane.normal = glm::vec3( 0.0f, 1.0f, 0.0f );
            break;
		case bounding_box_t::FACE_RIGHT:
			p = maxPoint;
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

	if ( oriented )
	{
		plane.normal = glm::normalize( glm::mat3( transform ) * plane.normal );
	}

    plane.d = glm::dot( p, plane.normal );
}

void bounding_box_t::SetDrawable( const glm::vec4& color_ )
{
    color = color_;
}

} // namespace geom
