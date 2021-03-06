#include "view.h"
#include "base.h"
#include "entity.h"

//-------------------------------------------------------------------------------------------------------
// Frustum
//-------------------------------------------------------------------------------------------------------
#define _DEBUG_FRUSTUM

view_frustum::view_frustum( void )
	:	mAcceptCount( 0 ),
		mRejectCount( 0 ),
		mMvp( 1.0f )
{
	memset( mFrustPlanes, 0, sizeof( plane ) * FRUST_NUM_PLANES );
}

view_frustum::~view_frustum( void )
{
}

glm::vec4 view_frustum::get_plane_from_origin( const glm::vec4& position, const glm::vec4& origin )
{
	glm::vec4 plane( 0.0f );
	plane.x = position.x;
	plane.y = position.y;
	plane.z = position.z;
	plane	= glm::normalize( plane );
	plane.w = glm::dot( glm::vec3( origin ), glm::vec3( position ) );

	return plane;
}

#define F_CalcDist( plane ) ( ( plane ).d / glm::length( ( plane ).normal ) )
#define F_CalcNormal( a, b ) ( glm::normalize( glm::cross( a, b ) ) )

// Update the frustum by computing the world-relative frustum of the camera.
// We calculaute the top, bottom, left, and right planes
// by taking the basis vectors of the camera's world-relative orientation
void view_frustum::update( const view_data& view )
{
	glm::mat4 inverseOrient(
		glm::normalize( view.mInverseOrient[ 0 ] ),
		glm::normalize( view.mInverseOrient[ 1 ] ),
		glm::normalize( view.mInverseOrient[ 2 ] ),
		glm::vec4( 0.0f, 0.0f, 0.0f, 1.0f )
	);

	float tanHalfFovy = glm::tan( view.mFovy * 0.5f );

	// We compute the reference angle since we want to base the sin/cosine on the angle from the x-axis;
	// without we have an angle from the z. // 0.75
	float fov = glm::atan( view.mAspect * 0.75f * tanHalfFovy );

	float fovDeg = glm::degrees( fov );
	UNUSEDPARAM( fovDeg );

	glm::vec3 u( inverseOrient[ 0 ] * glm::cos( fov ) );
	glm::vec3 v( inverseOrient[ 2 ] * -glm::sin( fov ) );
	glm::vec3 w( inverseOrient[ 1 ] );

	glm::vec3 planeLine( u + v );

	// add twice the view's origin to the line so that the camera's origin (which is represented with respect to the world)
	// is taken into account. This is effectively the same as
	// ( u + view.origin ) + ( v + view.origin ) = u + v + 2view.origin
	// Removing this addition is good for determining what is in view, but not for where the camera actually is.

	//planeLine += 2.0f * view.origin;

	mFrustPlanes[ FRUST_RIGHT ].mNormal = F_CalcNormal( planeLine, -w );
	mFrustPlanes[ FRUST_RIGHT ].mDistance = glm::dot( view.mOrigin + planeLine, mFrustPlanes[ FRUST_RIGHT ].mNormal );

	planeLine = -u + v;

	//planeLine += 2.0f * view.origin;

	mFrustPlanes[ FRUST_LEFT  ].mNormal = F_CalcNormal( planeLine, w );
	mFrustPlanes[ FRUST_LEFT  ].mDistance = glm::dot( view.mOrigin + planeLine, mFrustPlanes[ FRUST_LEFT ].mNormal );

	// Z is the initial axis for the horizontal planes
	fov = glm::atan( tanHalfFovy );

	u = -glm::vec3( inverseOrient[ 2 ] * glm::cos( fov ) );
	v = glm::vec3( inverseOrient[ 1 ] * -glm::sin( fov ) );
	w = glm::vec3( inverseOrient[ 0 ] );

	planeLine = -u + v;
	mFrustPlanes[ FRUST_TOP ].mNormal = F_CalcNormal( w, planeLine );
	mFrustPlanes[ FRUST_TOP ].mDistance = glm::dot( view.mOrigin + planeLine, mFrustPlanes[ FRUST_TOP ].mNormal );

	planeLine = u + v;
	mFrustPlanes[ FRUST_BOTTOM ].mNormal = F_CalcNormal( w, planeLine );
	mFrustPlanes[ FRUST_BOTTOM ].mDistance = glm::dot( view.mOrigin + planeLine, mFrustPlanes[ FRUST_BOTTOM ].mNormal );
}
#undef F_CalcNormal

// Adding plane[ 3 ] ( which is the distance from the plane to the origin ) offsets the plane so we can ensure that the point is in front of the plane normal

namespace {
	bool point_plane_predicate( float value )
	{
		return value >= 0.0f;
	}
}

bool view_frustum::intersects( const entity& e ) const
{
    UNUSEDPARAM( e );
    std::vector< glm::vec3 > ePoints;

    // const std::vector< glm::vec3 > ePoints( std::move( e.normal_body().world_space_points() ) );

	// Test each corner against every plane normal
	for ( int i = 0; i < 4; ++i )
	{
        if ( test_point_plane< point_plane_predicate >( ePoints, mFrustPlanes[ i ] ) )
		{
			continue;
		}

		mRejectCount++;
		return false;
	}

	mAcceptCount++;
	return true;
}

#ifdef F_PlaneSide
#	undef F_PlaneSide
#endif
