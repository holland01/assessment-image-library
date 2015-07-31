#include "view.h"
#include "base.h"
#include "geom.h"

#ifdef EMSCRIPTEN
#	define DEFAULT_MOVE_STEP 0.1f
#else
#	define DEFAULT_MOVE_STEP 1.0f
#endif


namespace view {

params_t::params_t( void )
    : forward( 0.0f ), up( 0.0f ), right( 0.0f ),
      origin( 0.0f ),
      fovy( 0.0f ), aspect( 0.0f ), zNear( 0.0f ), zFar( 0.0f ),
      width( 0.0f ), height( 0.0f ),
	  moveStep( DEFAULT_MOVE_STEP ),
      transform( 1.0f ),
      orientation( 1.0f ),
      inverseOrient( 1.0f ),
      clipTransform( 1.0f )
{
}

//-------------------------------------------------------------------------------------------------------
// Frustum
//-------------------------------------------------------------------------------------------------------
#define _DEBUG_FRUSTUM

frustum_t::frustum_t( void )
	:	acceptCount( 0 ),
		rejectCount( 0 ),
		mvp( 1.0f )
{
	memset( frustPlanes, 0, sizeof( geom::plane_t ) * FRUST_NUM_PLANES );
}

frustum_t::~frustum_t( void )
{
}

glm::vec4 frustum_t::CalcPlaneFromOrigin( const glm::vec4& position, const glm::vec4& origin )
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
void frustum_t::Update( const view::params_t& view )
{
	glm::mat4 inverseOrient(
		glm::normalize( view.inverseOrient[ 0 ] ),
		glm::normalize( view.inverseOrient[ 1 ] ),
		glm::normalize( view.inverseOrient[ 2 ] ),
		glm::vec4( 0.0f, 0.0f, 0.0f, 1.0f )
	);

	float tanHalfFovy = glm::tan( view.fovy * 0.5f );

	// We compute the reference angle since we want to base the sin/cosine on the angle from the x-axis;
	// without we have an angle from the z. // 0.75
	float fov = glm::atan( view.aspect * 0.75f * tanHalfFovy );

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

	frustPlanes[ FRUST_RIGHT ].normal = F_CalcNormal( planeLine, -w );
	frustPlanes[ FRUST_RIGHT ].d = glm::dot( view.origin + planeLine, frustPlanes[ FRUST_RIGHT ].normal );

	planeLine = -u + v;

	//planeLine += 2.0f * view.origin;

	frustPlanes[ FRUST_LEFT  ].normal = F_CalcNormal( planeLine, w );
	frustPlanes[ FRUST_LEFT  ].d = glm::dot( view.origin + planeLine, frustPlanes[ FRUST_LEFT ].normal );

	// Z is the initial axis for the horizontal planes
	fov = glm::atan( tanHalfFovy );

	u = -glm::vec3( inverseOrient[ 2 ] * glm::cos( fov ) );
	v = glm::vec3( inverseOrient[ 1 ] * -glm::sin( fov ) );
	w = glm::vec3( inverseOrient[ 0 ] );

	planeLine = -u + v;
	frustPlanes[ FRUST_TOP ].normal = F_CalcNormal( w, planeLine );
	frustPlanes[ FRUST_TOP ].d = glm::dot( view.origin + planeLine, frustPlanes[ FRUST_TOP ].normal );

	planeLine = u + v;
	frustPlanes[ FRUST_BOTTOM ].normal = F_CalcNormal( w, planeLine );
	frustPlanes[ FRUST_BOTTOM ].d = glm::dot( view.origin + planeLine, frustPlanes[ FRUST_BOTTOM ].normal );
}
#undef F_CalcNormal

// Adding plane[ 3 ] ( which is the distance from the plane to the origin ) offsets the plane so we can ensure that the point is in front of the plane normal

namespace {
	bool PointPlanePredicate( float value )
	{
		return value >= 0.0f;
	}
}

bool frustum_t::IntersectsBox( const geom::bounding_box_t& box ) const
{
	std::array< glm::vec3, 8 > clipBounds;
	box.GetPoints( clipBounds );

	// Test each corner against every plane normal
	for ( int i = 0; i < 4; ++i )
	{
		if ( geom::PointPlaneTest< 8, PointPlanePredicate >( clipBounds, frustPlanes[ i ] ) )
		{
			continue;
		}

		rejectCount++;
		return false;
	}

	acceptCount++;
	return true;
}

#ifdef F_PlaneSide
#	undef F_PlaneSide
#endif

} // namespace view
