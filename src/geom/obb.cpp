#include "obb.h"
#include "_geom_local.h"
#include "halfspace.h"
#include "geom.h"
#include "point_project_pair.h"

obb::obb( glm::mat3 transform_ )
	: bounds_primitive( BOUNDS_PRIM_BOX ),
	  mT( std::move( transform_ ) ),
	  mColor( glm::vec4( 1.0f ) )
{
}

obb::obb( obb&& m )
	: bounds_primitive( m ),
	  mT( std::move( m.mT ) ),
	  mColor( std::move( m.mColor ) )
{
}

obb::obb( const obb& c )
	: bounds_primitive( c ),
	  mT( c.mT ),
	  mColor( c.mColor )
{}

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

	int32_t edgeCount = 0;

	glm::mat3 ax( mT.scaled_axes() );

	for ( int32_t i = 0; i < 7; ++i )
	{
		if ( ( corner_type )i == index )
		{
			continue;
		}

		glm::vec3 edge( corner( ( corner_type ) i ) - origin );

		float edgeLen = glm::length( edge );

		float dx = glm::abs( glm::dot( edge, ax[ 0 ] ) );
		float dy = glm::abs( glm::dot( edge, ax[ 1 ] ) );
		float dz = glm::abs( glm::dot( edge, ax[ 2 ] ) );

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
	local::simd_intersect_convert_t c( {{ origin(), box.origin() }}, {{ axes(), box.axes() }} );
	local::intersect_comp_t test( c.vectors[ 0 ], c.matrices[ 0 ], c.vectors[ 1 ], c.matrices[ 1 ] );
#endif

	if ( !test.ValidateDistance() )
	{
		return false;
	}

	if ( !test.TestIntersection( test.origin, test.extents[ 0 ], test.srcOrigin, test.srcExtents ) ) return false;
	if ( !test.TestIntersection( test.origin, test.extents[ 1 ], test.srcOrigin, test.srcExtents ) ) return false;
	if ( !test.TestIntersection( test.origin, test.extents[ 2 ], test.srcOrigin, test.srcExtents ) ) return false;

	local::mat_t negAxis( test.srcExtents );

	if ( !test.TestIntersection( test.origin, test.extents[ 0 ], test.srcOrigin, -negAxis ) ) return false;
	if ( !test.TestIntersection( test.origin, test.extents[ 1 ], test.srcOrigin, -negAxis ) ) return false;
	if ( !test.TestIntersection( test.origin, test.extents[ 2 ], test.srcOrigin, -negAxis ) ) return false;

	return true;
}

bool obb::range( glm::vec3 v, bool inversed ) const
{
	if ( !inversed )
	{
		v = std::move( inv_axes() * v );
	}

	obb::maxmin_pair3D_t mm = std::move( maxmin( true ) );

	return glm::all( glm::lessThanEqual( mm.min, v ) )
		&& glm::all( glm::lessThanEqual( v, mm.max ) );
}

bool obb::intersects( contact::list_t& contacts, const obb& bounds ) const
{
	glm::vec3 toCenter( bounds.origin() - origin() );

	local::sat_intersection_test test( toCenter, mT, bounds.mT );

	if ( !local::has_intersection( test ) )
	{
		return false;
	}

	pointlist3D_t points;
	get_world_space_points( points );

	const glm::vec3 bCenter( bounds.origin() );

	for ( uint32_t f = 0; f < 6; ++f )
	{
		face_type face = ( face_type )f;

		plane Plane;
		bounds.face_plane( face, Plane );

		for ( const auto& p: points )
		{
			if ( Plane.has_point( p ) )
			{
				glm::vec3 normal( glm::normalize( bCenter - p ) );
				contact c( p, std::move( normal ) );
				contacts.insert( std::move( c ) );
			}
		}
	}

	return true;
}

bool obb::intersects( contact::list_t& contacts, const halfspace& halfSpace ) const
{
	if ( halfSpace.intersects( contacts, *this ) )
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
	glm::mat3 i( inv_axes() );
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

		//return range( rp, true );

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
		glm::mat3 i( inv_axes() );

		mm.max = i * mm.max;
		mm.min = i * mm.min;
	}

	return std::move( mm );
}

obb::pointset3D_t obb::face_project( const plane& facePlane, const pointlist3D_t& sourcePoints ) const
{
	pointset3D_t projected;

	std::vector< point_project_pair > replace;

	replace.reserve( sourcePoints.size() );

	for ( const auto& p: sourcePoints )
	{
		point_project_pair ppp( facePlane, p );

		auto i = projected.find( ppp );

		if ( i != projected.end() && ppp.closer_than( *i ) )
		{
			replace.push_back( ppp );
		}

		projected.insert( std::move( ppp ) );
	}

	std::vector< point_project_pair > tmp( projected.begin(), projected.end() );

	projected = std::move( obb::pointset3D_t( replace.begin(), replace.end() ) );
	projected.insert( tmp.begin(), tmp.end() );

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

	thePlane.mNormal = glm::normalize( axes() * thePlane.mNormal );
	thePlane.mReferencePoint = p;
	thePlane.mDistance = glm::dot( p, thePlane.mNormal );
}

void obb::color( const glm::vec4& color_ )
{
	mColor = color_;
}
