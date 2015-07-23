#include "view.h"
#include "base.h"

static const float MOUSE_SENSE = 0.1f;

#ifdef EMSCRIPTEN
#	define DEFAULT_MOVE_STEP 0.1f
#else
#	define DEFAULT_MOVE_STEP 0.01f
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

} // namespace view
