#include "input.h"
#include <glm/gtx/string_cast.hpp>
#include "bullet_ext.hpp"

static const float MOUSE_SENSE = 0.1f;

static INLINE void clamp_orientation( glm::vec3& r )
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
    : input_client( view_data() )
{
}

input_client::input_client( const view_data& view )
    : mViewParams( view ),
	  mMode( MODE_PLAY )
{
	mKeysPressed.fill( 0 );
}

input_client::input_client( float width, float height, const glm::mat4& viewTransform, const glm::mat4& projection )
    : input_client( view_data() )
{
    mViewParams.mOrigin = glm::vec3( -viewTransform[ 3 ] );
    mViewParams.mTransform = viewTransform;
    mViewParams.mClipTransform = projection;
    mViewParams.mOrientation = viewTransform;
    mViewParams.mOrientation[ 3 ] = glm::vec4( 0.0f, 0.0f, 0.0f, 1.0f );
    mViewParams.mInverseOrient = glm::inverse( mViewParams.mOrientation );
    mViewParams.mForward = forward();
    mViewParams.mUp = up();
    mViewParams.mRight = right();
    mViewParams.mWidth = width;
    mViewParams.mHeight = height;
}

void input_client::eval_mouse_move( float x, float y, bool calcRelative )
{
	if ( calcRelative )
	{
        mViewParams.mCurrRot.x += ( y - mViewParams.mLastMouse.y ) * MOUSE_SENSE;
        mViewParams.mCurrRot.y += ( x - mViewParams.mLastMouse.x ) * MOUSE_SENSE;
	}
	else
	{
        mViewParams.mCurrRot.x += y * MOUSE_SENSE;
        mViewParams.mCurrRot.y += x * MOUSE_SENSE;
	}

    mViewParams.mLastMouse.x = x;
    mViewParams.mLastMouse.y = y;
}

bool input_client::eval_key_press( input_key key )
{
    bool pressed = true;

	switch( key )
	{
		case input_key::W:
			mKeysPressed[ KEY_FORWARD ] = KEY_PRESSED;
			break;

		case input_key::S:
			mKeysPressed[ KEY_BACKWARD ] = KEY_PRESSED;
			break;

		case input_key::A:
			mKeysPressed[ KEY_LEFT ] = KEY_PRESSED;
			break;

		case input_key::D:
			mKeysPressed[ KEY_RIGHT ] = KEY_PRESSED;
			break;

		case input_key::LSHIFT:
			mKeysPressed[ KEY_DOWN ] = KEY_PRESSED;
			break;

		case input_key::SPACE:
			mKeysPressed[ KEY_UP ] = KEY_PRESSED;
			break;
		case input_key::E:
			mKeysPressed[ KEY_IN ] = KEY_PRESSED;
			break;
		case input_key::Q:
			mKeysPressed[ KEY_OUT ] = KEY_PRESSED;
			break;
		default:
            pressed = false;
			break;
	}

    return pressed;
}

bool input_client::eval_key_release( input_key key )
{
    bool pressed = true;

	switch( key )
	{
		case input_key::W:
			mKeysPressed[ KEY_FORWARD ] = KEY_NOT_PRESSED;
			break;
		case input_key::S:
			mKeysPressed[ KEY_BACKWARD ] = KEY_NOT_PRESSED;
			break;
		case input_key::A:
			mKeysPressed[ KEY_LEFT ] = KEY_NOT_PRESSED;
			break;
		case input_key::D:
			mKeysPressed[ KEY_RIGHT ] = KEY_NOT_PRESSED;
			break;
		case input_key::LSHIFT:
			mKeysPressed[ KEY_DOWN ] = KEY_NOT_PRESSED;
			break;
		case input_key::SPACE:
			mKeysPressed[ KEY_UP ] = KEY_NOT_PRESSED;
			break;
		case input_key::E:
			mKeysPressed[ KEY_IN ] = KEY_NOT_PRESSED;
			break;
		case input_key::Q:
			mKeysPressed[ KEY_OUT ] = KEY_NOT_PRESSED;
			break;
		default:
            pressed = false;
			break;
	}

    return pressed;
}

void input_client::add_dir( const glm::vec3& dir, float scale )
{
    glm::vec3 f( dir * scale );

    // spectating allows us to move throughout the world without interference of physics gravity
    physics_body* pb = get_body();

    if ( pb )
    {
        pb->mBody->translate( glm::ext::to_bullet( f ) );
    }
    else
    {
        mViewParams.mOrigin += f;
    }
}

void input_client::update_view_data( void )
{
    if ( mMode == MODE_PLAY )
    {
        clamp_orientation( mViewParams.mCurrRot );
    }

    mViewParams.mLastRot = mViewParams.mCurrRot;

    mViewParams.mLastOrientation = mViewParams.mOrientation;
    mViewParams.mOrientation = glm::rotate( glm::mat4( 1.0f ), glm::radians( mViewParams.mCurrRot.x ), glm::vec3( 1.0f, 0.0f, 0.0f ) );
    mViewParams.mOrientation = glm::rotate( mViewParams.mOrientation, glm::radians( mViewParams.mCurrRot.y ), glm::vec3( 0.0f, 1.0f, 0.0f ) );
    mViewParams.mOrientation = glm::rotate( mViewParams.mOrientation, glm::radians( mViewParams.mCurrRot.z ), glm::vec3( 0.0f, 0.0f, 1.0f ) );

    mViewParams.mInverseOrient = glm::inverse( mViewParams.mOrientation );

    mViewParams.mForward = forward();
    mViewParams.mRight = right();
    mViewParams.mUp = up();

    if ( mKeysPressed[ KEY_IN ] ) mViewParams.mCurrRot.z += mViewParams.mMoveStep;
    if ( mKeysPressed[ KEY_OUT ] ) mViewParams.mCurrRot.z -= mViewParams.mMoveStep;

    if ( mKeysPressed[ KEY_FORWARD ] ) add_dir( mViewParams.mForward, mViewParams.mMoveStep );
    if ( mKeysPressed[ KEY_BACKWARD ] ) add_dir( mViewParams.mForward, -mViewParams.mMoveStep );
    if ( mKeysPressed[ KEY_RIGHT ] ) add_dir( mViewParams.mRight, mViewParams.mMoveStep );
    if ( mKeysPressed[ KEY_LEFT ] ) add_dir( mViewParams.mRight, -mViewParams.mMoveStep );
    if ( mKeysPressed[ KEY_UP ] ) add_dir( mViewParams.mUp, mViewParams.mMoveStep );
    if ( mKeysPressed[ KEY_DOWN ] ) add_dir( mViewParams.mUp, -mViewParams.mMoveStep );

    if ( mMode == MODE_PLAY )
    {
        mViewParams.mOrigin.y = 0.0f;
    }

    if ( !mPhysEnt )
    {
        mViewParams.mTransform = mViewParams.mOrientation * glm::translate( glm::mat4( 1.0f ), -mViewParams.mOrigin );
    }
}

void input_client::sync( void )
{
    physics_body* body = get_body();

    if ( body )
    {
        update_view_data();

        glm::mat4 gBasis( 1.0f );

        if ( body->mMotionState )
        {
            const btMotionState& ms = body->motion_state();

            btTransform worldT;
            ms.getWorldTransform( worldT );

            gBasis = glm::ext::from_bullet( worldT );
        }
        else
        {
            gBasis = glm::ext::from_bullet( body->mBody->getCenterOfMassTransform() );
        }

        mViewParams.mTransform = mViewParams.mOrientation * glm::inverse( gBasis );
        mViewParams.mOrigin = glm::vec3( gBasis[ 3 ] );
    }
    else
    {
        update_view_data();
    }

    entity::sync();
}

void input_client::set_physics( float mass, const glm::mat4& orientAndTranslate )
{
    mPhysEnt.reset( new physics_body( mass, orientAndTranslate, glm::vec3( 1.0f ) ) );
    mPhysEnt->set_activation_state( DISABLE_DEACTIVATION );
}

void input_client::print_origin( void ) const
{
    printf( "Origin: %s\n", glm::to_string( mViewParams.mOrigin ).c_str() );
}
