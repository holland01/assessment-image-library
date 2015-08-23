#include "input.h"
#include <glm/gtx/string_cast.hpp>

static const float MOUSE_SENSE = 0.1f;

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

input_client::input_client( void )
    : input_client( view_params_t() )
{
}

input_client::input_client( const view_params_t& view )
    : entity( entity::BODY_DEPENDENT,
                []( void ) -> rigid_body*
                {
                    return nullptr;
                }() ),
      viewParams( view ),
	  mode( MODE_PLAY )
{
	keysPressed.fill( 0 );
    add_bounds( ENTITY_BOUNDS_ALL, new obb() );
}

input_client::input_client( float width, float height, const glm::mat4& viewTransform, const glm::mat4& projection )
    : input_client( view_params_t() )
{
	viewParams.origin = glm::vec3( -viewTransform[ 3 ] );
	viewParams.transform = viewTransform;
	viewParams.clipTransform = projection;
	viewParams.orientation = viewTransform;
	viewParams.orientation[ 3 ] = glm::vec4( 0.0f, 0.0f, 0.0f, 1.0f );
	viewParams.inverseOrient = glm::inverse( viewParams.orientation );
	viewParams.forward = Forward();
	viewParams.up = Up();
	viewParams.right = Right();
	viewParams.width = width;
	viewParams.height = height;
}

void input_client::EvalMouseMove( float x, float y, bool calcRelative )
{
	if ( calcRelative )
	{
		viewParams.currRot.x += ( y - viewParams.lastMouse.y ) * MOUSE_SENSE;
		viewParams.currRot.y += ( x - viewParams.lastMouse.x ) * MOUSE_SENSE;
	}
	else
	{
		viewParams.currRot.x += y * MOUSE_SENSE;
		viewParams.currRot.y += x * MOUSE_SENSE;
	}

	viewParams.lastMouse.x = x;
	viewParams.lastMouse.y = y;
}

void input_client::EvalKeyPress( input_key key )
{
	switch( key )
	{
		case input_key::W:
			keysPressed[ KEY_FORWARD ] = KEY_PRESSED;
			break;

		case input_key::S:
			keysPressed[ KEY_BACKWARD ] = KEY_PRESSED;
			break;

		case input_key::A:
			keysPressed[ KEY_LEFT ] = KEY_PRESSED;
			break;

		case input_key::D:
			keysPressed[ KEY_RIGHT ] = KEY_PRESSED;
			break;

		case input_key::LSHIFT:
			keysPressed[ KEY_DOWN ] = KEY_PRESSED;
			break;

		case input_key::SPACE:
			keysPressed[ KEY_UP ] = KEY_PRESSED;
			break;
		case input_key::E:
			keysPressed[ KEY_IN ] = KEY_PRESSED;
			break;
		case input_key::Q:
			keysPressed[ KEY_OUT ] = KEY_PRESSED;
			break;
		default:
			break;
	}
}

void input_client::EvalKeyRelease( input_key key )
{
	switch( key )
	{
		case input_key::W:
			keysPressed[ KEY_FORWARD ] = KEY_NOT_PRESSED;
			break;
		case input_key::S:
			keysPressed[ KEY_BACKWARD ] = KEY_NOT_PRESSED;
			break;
		case input_key::A:
			keysPressed[ KEY_LEFT ] = KEY_NOT_PRESSED;
			break;
		case input_key::D:
			keysPressed[ KEY_RIGHT ] = KEY_NOT_PRESSED;
			break;
		case input_key::LSHIFT:
			keysPressed[ KEY_DOWN ] = KEY_NOT_PRESSED;
			break;
		case input_key::SPACE:
			keysPressed[ KEY_UP ] = KEY_NOT_PRESSED;
			break;
		case input_key::E:
			keysPressed[ KEY_IN ] = KEY_NOT_PRESSED;
			break;
		case input_key::Q:
			keysPressed[ KEY_OUT ] = KEY_NOT_PRESSED;
			break;
		default:
			break;

	}
}

void input_client::ApplyMovement( void )
{
	NormalizeRotation( viewParams.currRot );

	viewParams.lastRot = viewParams.currRot;

	viewParams.forward = Forward();
	viewParams.right = Right();
	viewParams.up = Up();

	if ( keysPressed[ KEY_FORWARD ] ) AddDir( viewParams.forward, viewParams.moveStep );
	if ( keysPressed[ KEY_BACKWARD ] ) AddDir( viewParams.forward, -viewParams.moveStep );
	if ( keysPressed[ KEY_RIGHT ] ) AddDir( viewParams.right, viewParams.moveStep );
	if ( keysPressed[ KEY_LEFT ] ) AddDir( viewParams.right, -viewParams.moveStep );
	if ( keysPressed[ KEY_UP ] ) AddDir( viewParams.up, viewParams.moveStep );
	if ( keysPressed[ KEY_DOWN ] ) AddDir( viewParams.up, -viewParams.moveStep );
	if ( keysPressed[ KEY_IN ] ) viewParams.currRot.z += viewParams.moveStep;
	if ( keysPressed[ KEY_OUT ] ) viewParams.currRot.z -= viewParams.moveStep;
}

void input_client::sync( void )
{
	viewParams.orientation = glm::rotate( glm::mat4( 1.0f ), glm::radians( viewParams.currRot.x ), glm::vec3( 1.0f, 0.0f, 0.0f ) );
	viewParams.orientation = glm::rotate( viewParams.orientation, glm::radians( viewParams.currRot.y ), glm::vec3( 0.0f, 1.0f, 0.0f ) );
	viewParams.orientation = glm::rotate( viewParams.orientation, glm::radians( viewParams.currRot.z ), glm::vec3( 0.0f, 0.0f, 1.0f ) );

    viewParams.inverseOrient = glm::inverse( viewParams.orientation );

    viewParams.transform = viewParams.orientation * glm::translate( glm::mat4( 1.0f ), -viewParams.origin );

	if ( mode == MODE_PLAY )
	{
		if ( mBody )
		{
            mBody->position( 1.0f, 0.0f );
		}
		else
		{
			viewParams.origin.y = 0.0f;
		}
	}

	if ( mBody )
	{
        viewParams.origin = mBody->position();
        mBody->orientation( viewParams.inverseOrient );
        entity::sync();
	}
	else
	{
        obb* b = query_bounds( ENTITY_BOUNDS_ALL )->to_box();
        assert( b );

        b->center( viewParams.origin );
        b->orientation( glm::mat3( viewParams.inverseOrient ) );
	}
}

void input_client::PrintOrigin( void ) const
{
	printf( "Origin: %s\n", glm::to_string( viewParams.origin ).c_str() );
}
