#include "_geom_local.h"
#include "transform_data.h"

namespace {

float penetration_depth( const glm::vec3& axis, const detail::sat_intersection_test& test )
{
	auto project_onto_axis = [ &axis ]( const glm::mat3& linAxes, const glm::vec3& extents ) -> float
	{
		glm::vec3 s( glm::dot( axis, linAxes[ 0 ] ),
					glm::dot( axis, linAxes[ 1 ] ),
					glm::dot( axis, linAxes[ 2 ] ) );

		return glm::dot( extents, glm::abs( s ) );
	};

	float aproj = project_onto_axis( test.mA.mAxes, test.mA.mExtents );
	float bproj = project_onto_axis( test.mB.mAxes, test.mB.mExtents );

	float centerProj = glm::abs( glm::dot( axis, test.mToCenter ) );

	return ( aproj + bproj ) - centerProj;
}

bool eval_axis( uint32_t index, detail::sat_intersection_test& test, const glm::vec3& axis )
{
	float depth = penetration_depth( axis, test );

	if ( depth < 0.0f )
	{
		return false;
	}

	if ( depth < test.mSmallestPenetration )
	{
		test.mSmallestPenetrationIndex = index;
		test.mSmallestPenetration = depth;
	}

	return true;
}

} // end namespace

namespace detail {

sat_intersection_test::sat_intersection_test( const transform_data& a, const transform_data& b )
	: sat_intersection_test( b.mOrigin - a.mOrigin, a, b )
{
}

sat_intersection_test::sat_intersection_test( const glm::vec3& toCenter,
											  const transform_data& a,
											  const transform_data& b )
	: mToCenter( toCenter ),
	  mA( a ),
	  mB( b ),
	  mBestSingleAxisSPIndex( 0 ),
	  mSmallestPenetrationIndex( 0 ),
	  mSmallestPenetration( FLT_MAX )
{
}

#define TEST_SEPARATING_AXIS( axis, index ) if ( !eval_axis( ( index ), *this, ( axis ) ) ) return false;
bool sat_intersection_test::operator()( void )
{
	TEST_SEPARATING_AXIS( mA.mAxes[ 0 ], 0 )
	TEST_SEPARATING_AXIS( mA.mAxes[ 1 ], 1 )
	TEST_SEPARATING_AXIS( mA.mAxes[ 2 ], 2 )

	TEST_SEPARATING_AXIS( mB.mAxes[ 0 ], 3 )
	TEST_SEPARATING_AXIS( mB.mAxes[ 1 ], 4 )
	TEST_SEPARATING_AXIS( mB.mAxes[ 2 ], 5 )

	mBestSingleAxisSPIndex = mSmallestPenetrationIndex;

	TEST_SEPARATING_AXIS( glm::cross( mA.mAxes[ 0 ], mB.mAxes[ 0 ] ), 6 )
	TEST_SEPARATING_AXIS( glm::cross( mA.mAxes[ 0 ], mB.mAxes[ 1 ] ), 7 )
	TEST_SEPARATING_AXIS( glm::cross( mA.mAxes[ 0 ], mB.mAxes[ 2 ] ), 8 )

	TEST_SEPARATING_AXIS( glm::cross( mA.mAxes[ 1 ], mB.mAxes[ 0 ] ), 9 )
	TEST_SEPARATING_AXIS( glm::cross( mA.mAxes[ 1 ], mB.mAxes[ 1 ] ), 10 )
	TEST_SEPARATING_AXIS( glm::cross( mA.mAxes[ 1 ], mB.mAxes[ 2 ] ), 11 )

	TEST_SEPARATING_AXIS( glm::cross( mA.mAxes[ 2 ], mB.mAxes[ 0 ] ), 12 )
	TEST_SEPARATING_AXIS( glm::cross( mA.mAxes[ 2 ], mB.mAxes[ 1 ] ), 13 )
	TEST_SEPARATING_AXIS( glm::cross( mA.mAxes[ 2 ], mB.mAxes[ 2 ] ), 14 )

	return true;
}
#undef TEST_SEPARATING_AXIS

//------------------------------------------------------------

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

//------------------------------------------------------------

} // namespace local
