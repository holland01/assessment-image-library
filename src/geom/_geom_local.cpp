#include "_geom_local.h"
#include "transform_data.h"

namespace local {

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

float penetration_depth( const glm::vec3& axis, const sat_intersection_test& test )
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

bool eval_axis( uint32_t index, sat_intersection_test& test, const glm::vec3& axis )
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

#define TEST_SEPARATING_AXIS( axis, index ) if ( !eval_axis( ( index ), test, ( axis ) ) ) return false;
bool has_intersection( sat_intersection_test& test )
{
	TEST_SEPARATING_AXIS( test.mA.mAxes[ 0 ], 0 )
	TEST_SEPARATING_AXIS( test.mA.mAxes[ 1 ], 1 )
	TEST_SEPARATING_AXIS( test.mA.mAxes[ 2 ], 2 )

	TEST_SEPARATING_AXIS( test.mB.mAxes[ 0 ], 3 )
	TEST_SEPARATING_AXIS( test.mB.mAxes[ 1 ], 4 )
	TEST_SEPARATING_AXIS( test.mB.mAxes[ 2 ], 5 )

	test.mBestSingleAxisSPIndex = test.mSmallestPenetrationIndex;

	TEST_SEPARATING_AXIS( glm::cross( test.mA.mAxes[ 0 ], test.mB.mAxes[ 0 ] ), 6 )
	TEST_SEPARATING_AXIS( glm::cross( test.mA.mAxes[ 0 ], test.mB.mAxes[ 1 ] ), 7 )
	TEST_SEPARATING_AXIS( glm::cross( test.mA.mAxes[ 0 ], test.mB.mAxes[ 2 ] ), 8 )

	TEST_SEPARATING_AXIS( glm::cross( test.mA.mAxes[ 1 ], test.mB.mAxes[ 0 ] ), 9 )
	TEST_SEPARATING_AXIS( glm::cross( test.mA.mAxes[ 1 ], test.mB.mAxes[ 1 ] ), 10 )
	TEST_SEPARATING_AXIS( glm::cross( test.mA.mAxes[ 1 ], test.mB.mAxes[ 2 ] ), 11 )

	TEST_SEPARATING_AXIS( glm::cross( test.mA.mAxes[ 2 ], test.mB.mAxes[ 0 ] ), 12 )
	TEST_SEPARATING_AXIS( glm::cross( test.mA.mAxes[ 2 ], test.mB.mAxes[ 1 ] ), 13 )
	TEST_SEPARATING_AXIS( glm::cross( test.mA.mAxes[ 2 ], test.mB.mAxes[ 2 ] ), 14 )

	return true;
}
#undef TEST_SEPARATING_AXIS

} // namespace local
