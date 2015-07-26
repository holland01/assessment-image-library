#include "view.h"
#include "base.h"
#include "geom.h"
#include <glm/gtx/string_cast.hpp>

static const float MOUSE_SENSE = 0.1f;

#ifdef EMSCRIPTEN
#	define DEFAULT_MOVE_STEP 0.1f
#else
#	define DEFAULT_MOVE_STEP 0.1f
#endif

static INLINE void NormalizeRotation( glm::vec3& r )
{
    if ( r.x > 89.9f )
    {
        r.x -= 180.0f;
    }
    else if ( r.x < -89.9f )
    {
        r.x += 180.0f;
    }

    if ( r.y > 180.0f )
    {
        r.y -= 360.0f;
    }
    else if ( r.y < -180.0f )
    {
        r.y += 360.0f;
    }
}

enum
{
    KEY_PRESSED = 1,
    KEY_NOT_PRESSED = 0,
    KEY_FORWARD = 0,
    KEY_BACKWARD = 1,
    KEY_LEFT = 2,
    KEY_RIGHT = 3,
    KEY_UP = 4,
    KEY_DOWN = 5,
    KEY_IN = 6,
    KEY_OUT = 7
};


namespace view {

params_t::params_t( void )
    : forward( 0.0f ), up( 0.0f ), right( 0.0f ),
      origin( 0.0f ),
      fovy( 0.0f ), aspect( 0.0f ), zNear( 0.0f ), zFar( 0.0f ),
      width( 0.0f ), height( 0.0f ),
      transform( 1.0f ),
      orientation( 1.0f ),
      inverseOrient( 1.0f ),
      clipTransform( 1.0f )
{
}

camera_t::camera_t( void )
    : camera_t( params_t(), glm::vec3( 0.0f ) )
{
}

camera_t::camera_t( const params_t& view, const glm::vec3& currRot )
    : viewParams( view ),
      currRot( currRot ),
      lastMouse( 0.0f ),
	  moveStep( DEFAULT_MOVE_STEP )
{
    keysPressed.fill( 0 );
}

camera_t::camera_t( float width, float height, const glm::mat4& view, const glm::mat4& projection )
{
    viewParams.origin = glm::vec3( -view[ 3 ] );
    viewParams.transform = view;
    viewParams.clipTransform = projection;
    viewParams.orientation = view;
    viewParams.orientation[ 3 ] = glm::vec4( 0.0f, 0.0f, 0.0f, 1.0f );
    viewParams.inverseOrient = glm::inverse( viewParams.orientation );
    viewParams.forward = Forward();
    viewParams.up = Up();
    viewParams.right = Right();
    viewParams.width = width;
    viewParams.height = height;
}

void camera_t::EvalMouseMove( float x, float y, bool calcRelative )
{
    if ( calcRelative )
    {
        currRot.x += ( y - lastMouse.y ) * MOUSE_SENSE;
        currRot.y += ( x - lastMouse.x ) * MOUSE_SENSE;
    }
    else
    {
        currRot.x += y * MOUSE_SENSE;
        currRot.y += x * MOUSE_SENSE;
    }

    lastMouse.x = x;
    lastMouse.y = y;
}

void camera_t::EvalKeyPress( input_key_t key )
{
    switch( key )
    {
        case input_key_t::W:
            keysPressed[ KEY_FORWARD ] = KEY_PRESSED;
            break;

        case input_key_t::S:
            keysPressed[ KEY_BACKWARD ] = KEY_PRESSED;
            break;

        case input_key_t::A:
            keysPressed[ KEY_LEFT ] = KEY_PRESSED;
            break;

        case input_key_t::D:
            keysPressed[ KEY_RIGHT ] = KEY_PRESSED;
            break;

        case input_key_t::LSHIFT:
            keysPressed[ KEY_DOWN ] = KEY_PRESSED;
            break;

        case input_key_t::SPACE:
            keysPressed[ KEY_UP ] = KEY_PRESSED;
            break;
        case input_key_t::E:
            keysPressed[ KEY_IN ] = KEY_PRESSED;
            break;
        case input_key_t::Q:
            keysPressed[ KEY_OUT ] = KEY_PRESSED;
            break;
        default:
            break;
    }
}

void camera_t::EvalKeyRelease( input_key_t key )
{
    switch( key )
    {
        case input_key_t::W:
            keysPressed[ KEY_FORWARD ] = KEY_NOT_PRESSED;
            break;
        case input_key_t::S:
            keysPressed[ KEY_BACKWARD ] = KEY_NOT_PRESSED;
            break;
        case input_key_t::A:
            keysPressed[ KEY_LEFT ] = KEY_NOT_PRESSED;
            break;
        case input_key_t::D:
            keysPressed[ KEY_RIGHT ] = KEY_NOT_PRESSED;
            break;
        case input_key_t::LSHIFT:
            keysPressed[ KEY_DOWN ] = KEY_NOT_PRESSED;
            break;
        case input_key_t::SPACE:
            keysPressed[ KEY_UP ] = KEY_NOT_PRESSED;
            break;
        case input_key_t::E:
            keysPressed[ KEY_IN ] = KEY_NOT_PRESSED;
            break;
        case input_key_t::Q:
            keysPressed[ KEY_OUT ] = KEY_NOT_PRESSED;
            break;
        default:
            break;

    }
}

void camera_t::Update( void )
{
     NormalizeRotation( currRot );

     lastRot = currRot;

	 viewParams.forward = Forward();
	 viewParams.right = Right();
	 viewParams.up = Up();

    if ( keysPressed[ KEY_FORWARD ] ) Walk( moveStep );
    if ( keysPressed[ KEY_BACKWARD ] ) Walk( -moveStep );
    if ( keysPressed[ KEY_RIGHT ] ) Strafe( moveStep );
    if ( keysPressed[ KEY_LEFT ] ) Strafe( -moveStep );
    if ( keysPressed[ KEY_UP ] ) Raise( moveStep );
    if ( keysPressed[ KEY_DOWN ] ) Raise( -moveStep );
    if ( keysPressed[ KEY_IN ] ) currRot.z += moveStep;
    if ( keysPressed[ KEY_OUT ] ) currRot.z -= moveStep;

	viewParams.orientation = glm::rotate( glm::mat4( 1.0f ), glm::radians( currRot.x ), glm::vec3( 1.0f, 0.0f, 0.0f ) );
	viewParams.orientation = glm::rotate( viewParams.orientation, glm::radians( currRot.y ), glm::vec3( 0.0f, 1.0f, 0.0f ) );
    viewParams.orientation = glm::rotate( viewParams.orientation, glm::radians( currRot.z ), glm::vec3( 0.0f, 0.0f, 1.0f ) );

    viewParams.inverseOrient = glm::inverse( viewParams.orientation );

    viewParams.transform = viewParams.orientation * glm::translate( glm::mat4( 1.0f ), -viewParams.origin );
}

void camera_t::PrintOrigin( void ) const
{
    printf( "Origin: %s\n", glm::to_string( viewParams.origin ).c_str() );
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

#ifdef _DEBUG_FRUSTUM
	static float F_PlaneSide( const glm::vec3& point, const geom::plane_t& plane )
	{
		float x = glm::dot( point, plane.normal ) - plane.d;

		return x;
	}
#else
#	define F_PlaneSide( point, plane ) ( glm::dot( ( point ), ( plane ).normal ) - ( plane ).d )
)
#endif

bool frustum_t::IntersectsBox( const geom::bounding_box_t& box ) const
{
#define C(v) ( glm::vec3( ( v ) ) )

	std::array< glm::vec3, 8 > clipBounds =
	{{
		C( box.Corner4( 0 ) ),
		C( box.Corner4( 1 ) ),
		C( box.Corner4( 2 ) ),
		C( box.Corner4( 3 ) ),
		C( box.Corner4( 4 ) ),
		C( box.Corner4( 5 ) ),
		C( box.Corner4( 6 ) ),
		C( box.Corner4( 7 ) )
	}};
#undef C

	// Test each corner against every plane normal
	for ( int i = 0; i < 4; ++i )
	{
		if ( F_PlaneSide( clipBounds[ 0 ], frustPlanes[ i ] ) >= 0 ) continue;
		if ( F_PlaneSide( clipBounds[ 1 ], frustPlanes[ i ] ) >= 0 ) continue;
		if ( F_PlaneSide( clipBounds[ 2 ], frustPlanes[ i ] ) >= 0 ) continue;
		if ( F_PlaneSide( clipBounds[ 3 ], frustPlanes[ i ] ) >= 0 ) continue;
		if ( F_PlaneSide( clipBounds[ 4 ], frustPlanes[ i ] ) >= 0 ) continue;
		if ( F_PlaneSide( clipBounds[ 5 ], frustPlanes[ i ] ) >= 0 ) continue;
		if ( F_PlaneSide( clipBounds[ 6 ], frustPlanes[ i ] ) >= 0 ) continue;
		if ( F_PlaneSide( clipBounds[ 7 ], frustPlanes[ i ] ) >= 0 ) continue;

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
