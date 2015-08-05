
#ifdef EMSCRIPTEN

#include <emscripten.h>
#include <html5.h>

void App_Frame( void );

namespace {

const std::vector< input_key_t > emKeyMap =
{
    input_key_t::W,
    input_key_t::S,
    input_key_t::A,
    input_key_t::D,
    input_key_t::E,
    input_key_t::Q,
    input_key_t::R,
    input_key_t::V,
    input_key_t::ESC,
    input_key_t::SPACE,
    input_key_t::LSHIFT
};

INLINE std::string EmscriptenResultFromEnum( int32_t result )
{
	switch ( result )
	{
		case EMSCRIPTEN_RESULT_SUCCESS: return "EMSCRIPTEN_RESULT_SUCCESS";
		case EMSCRIPTEN_RESULT_DEFERRED: return "EMSCRIPTEN_RESULT_DEFERRED";
		case EMSCRIPTEN_RESULT_NOT_SUPPORTED: return "EMSCRIPTEN_RESULT_NOT_SUPPORTED";
		case EMSCRIPTEN_RESULT_FAILED_NOT_DEFERRED: "EMSCRIPTEN_RESULT_FAILED_NOT_DEFERRED";
		case EMSCRIPTEN_RESULT_INVALID_TARGET: return "EMSCRIPTEN_RESULT_INVALID_TARGET";
		case EMSCRIPTEN_RESULT_UNKNOWN_TARGET: return "EMSCRIPTEN_RESULT_UNKNOWN_TARGET";
		case EMSCRIPTEN_RESULT_INVALID_PARAM: return "EMSCRIPTEN_RESULT_INVALID_PARAM";
		case EMSCRIPTEN_RESULT_FAILED: return "EMSCRIPTEN_RESULT_FAILED";
		case EMSCRIPTEN_RESULT_NO_DATA: return "EMSCRIPTEN_RESULT_NO_DATA";
	}

	return "EMSCRIPTEN_RESULT_UNDEFINED";
}

#define SET_CALLBACK_RESULT( expr )\
	do\
	{\
		ret = ( expr );\
		if ( ret != EMSCRIPTEN_RESULT_SUCCESS )\
		{\
			MLOG_ERROR( "Error setting emscripten function. Result returned: %s, Call expression: %s", EmscriptenResultFromEnum( ret ).c_str(), #expr );\
		}\
	}\
	while( 0 )

typedef void ( input_client_t::*camKeyFunc_t )( input_key_t key );

template< camKeyFunc_t cameraKeyFunc >
INLINE EM_BOOL KeyInputFunc( int32_t eventType, const EmscriptenKeyboardEvent* keyEvent, void* userData )
{
	UNUSEDPARAM( userData );

	game_t& app = game_t::GetInstance();

    auto entry = std::find( emKeyMap.begin(), emKeyMap.end(), ( input_key_t ) keyEvent->keyCode );

	if ( entry == emKeyMap.end() )
	{
		return 0;
	}

    switch ( *entry )
	{
		case input_key_t::ESC:
			app.running = false;
			break;
		case input_key_t::R:
			if ( eventType == EMSCRIPTEN_EVENT_KEYDOWN )
			{
				app.ResetMap();
			}
			break;
		case input_key_t::V:
			if ( eventType == EMSCRIPTEN_EVENT_KEYDOWN )
			{
				app.ToggleCulling();
			}
			break;
		default:
            CALL_MEM_FNPTR( *app.camera, cameraKeyFunc )( *entry );
			break;
	}

	return 1;
}

EM_BOOL MouseMoveFunc( int32_t eventType, const EmscriptenMouseEvent* mouseEvent, void* userData )
{
	UNUSEDPARAM( eventType );
	UNUSEDPARAM( userData );

	game_t& app = game_t::GetInstance();

	EmscriptenPointerlockChangeEvent pl;
	emscripten_get_pointerlock_status( &pl );

	bool activePl = !!pl.isActive;

    // If pointer lock is actually active, give a shit and do something about it...
	if ( activePl )
	{
		app.camera->EvalMouseMove( ( float ) mouseEvent->movementX, ( float ) mouseEvent->movementY, false );
	}

	return 1;
}

EM_BOOL MouseDownFunc( int32_t eventType, const EmscriptenMouseEvent* mouseEvent, void* userData )
{
    UNUSEDPARAM( eventType );
    UNUSEDPARAM( userData );

    game_t& app = game_t::GetInstance();

    if ( mouseEvent->button == 0 )
    {
        app.FireGun();
    }

    return 1;
}

void InitEmInput( void )
{
	int32_t ret;
	SET_CALLBACK_RESULT( emscripten_set_keydown_callback( nullptr, nullptr, 0, ( em_key_callback_func )&KeyInputFunc< &input_client_t::EvalKeyPress > ) );
	SET_CALLBACK_RESULT( emscripten_set_keyup_callback( nullptr, nullptr, 0, ( em_key_callback_func )&KeyInputFunc< &input_client_t::EvalKeyRelease > ) );
	SET_CALLBACK_RESULT( emscripten_set_mousemove_callback( "#canvas", nullptr, 1, ( em_mouse_callback_func )&MouseMoveFunc ) );
    SET_CALLBACK_RESULT( emscripten_set_mousedown_callback( "#canvas", nullptr, 1, ( em_mouse_callback_func )&MouseDownFunc ) );

	emscripten_set_main_loop( ( em_callback_func )&Game_Frame, 0, 1 );
}

}

#endif // EMSCRIPTEN
