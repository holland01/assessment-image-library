#include "halfspace.h"
#include "../renderer.h"
#include "obb.h"
#include "geom_util.h"

halfspace::halfspace( void )
	: halfspace( glm::mat3( 1.0f ), glm::vec3( 0.0f ), 0.0f )
{
}

halfspace::halfspace( const glm::mat3& axes, const glm::vec3& origin, float distance_ )
	: bounds_primitive( BOUNDS_PRIM_HALFSPACE ),
	  mT( axes, origin ),
	  mDistance( distance_ )
{
}

halfspace::halfspace( const obb& bounds, const glm::vec3& normal )
	: halfspace( glm::mat3( 1.0f ), glm::vec3( 0.0f ), 0.0f )
{
	using corner_t = obb::corner_type;

	glm::vec3 upAxis( bounds.axes()[ 1 ] );
	glm::vec3 boundsOrigin( bounds.origin() );
	glm::vec3 boundsSize( bounds.extents() * 2.0f );

	// normalize the cross on mAxes[ 0 ] so that we don't scale more than is necessary
	mT.mAxes[ 0 ] = std::move( glm::normalize( glm::cross( normal, upAxis ) ) ) * boundsSize[ 0 ];
	mT.mAxes[ 1 ] = upAxis * boundsSize[ 1 ];
	mT.mAxes[ 2 ] = normal;

	// normal is assumed to be a cardinal (positive or negative) axis, so multiplying
	// by boundsSizes will distribute the intended length along the appropriate component, where as all others will be zero
	// as before. We then transform the scaled normal, since scaling before transformation will keep the length preserved (as opposed to after).
	glm::vec3 faceCenter( bounds.axes() * ( glm::normalize( normal ) * boundsSize * 0.5f ) );
	faceCenter += boundsOrigin; // offset by boundsOrigin since we need its worldSpace position

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
		glm::vec3 point( bounds.world_corner( face ) );
		glm::vec3 pointToCenter( faceCenter - point );

		// Not in same plane; move on
		if ( triple_product( pointToCenter, mT.mAxes[ 0 ], mT.mAxes[ 1 ] ) != 0.0f )
		{
			continue;
		}

		// Half space axes will be outside of the bounds; move on
		if ( !bounds.range( point + mT.mAxes[ 0 ], false ) || !bounds.range( point + mT.mAxes[ 1 ], false ) )
		{
			continue;
		}

		mT.mOrigin = point;

		break;
	}

	mDistance = glm::dot( glm::normalize( mT.mAxes[ 2 ] ), mT.mOrigin );
}

bool halfspace::intersects( contact::list_t& contacts, const obb& bounds ) const
{
	UNUSEDPARAM( contacts );

	glm::vec3 normal( glm::normalize( mT.mAxes[ 1 ] ) );
	plane P( normal, mT.mOrigin );

	// Project toCenter onto plane which has the same normal as the direction of the halfspace up vector (mT.mAxes[1]) as a normal
	// This will prevent the SAT from fucking up, since the origins are typically in the bottom corner of the halfspace and the bounds' origins
	// are directly in the middle
	glm::vec3 projToCenter( P.project( bounds.origin() ) - mT.mOrigin );

	//projToCenter = bounds.axes() * projToCenter;

	detail::sat_intersection_test test( projToCenter, mT.transform_to( bounds.trans_data() ), bounds.trans_data() );
	return test();
}

void halfspace::draw( imm_draw& drawer ) const
{
	drawer.begin( GL_LINES );

	drawer.vertex( origin() );
	drawer.vertex( origin() + axes()[ 0 ] );

	drawer.vertex( origin() );
	drawer.vertex( origin() + axes()[ 1 ] );

	drawer.vertex( origin() );
	drawer.vertex( origin() + axes()[ 2 ] * 2.0f );

	drawer.end();

	drawer.begin( GL_POINTS );
	drawer.vertex( origin() );
	drawer.end();
}
