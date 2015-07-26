//#include "def.h"

/*
#ifndef EMSCRIPTEN
#   define EMSCRIPTEN
#endif
*/

#include <iostream>
#ifdef EMSCRIPTEN
#   include <emscripten.h>
#   include <html5.h>
#endif

#include "renderer.h"
#include "geom.h"
#include "view.h"
#include "map.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_main.h>

#ifdef OP_UNIX
	#include <unistd.h>
#endif

#include <stdint.h>
#include <array>
#include <cstdlib>
#include <unordered_map>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_access.hpp>

#define VERT( p, c ) { p, glm::vec3( 0.0f ), glm::vec2( 0.0f ), c }

bool Predicate( geom::bounding_box_t& bounds );

namespace {

struct app_t
{
	bool running = false, mouseShown = true, drawAll = false;
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_GLContext context = nullptr;

    uint32_t width, height;

	std::unique_ptr< map::generator_t > gen;
	std::unique_ptr< rend::pipeline_t > pipeline;

	GLuint globalVao;

    view::camera_t         camera;
	view::frustum_t		   frustum;

    app_t( uint32_t width, uint32_t height );
   ~app_t( void );

	void ResetMap( void );
	void ToggleCulling( void );

    static app_t& GetInstance( void );
};

#ifdef EMSCRIPTEN
const std::unordered_map< std::string, input_key_t > emKeyMap =
{
    { "KeyW", input_key_t::W },
    { "KeyS", input_key_t::S },
    { "KeyA", input_key_t::A },
    { "KeyD", input_key_t::D },
    { "KeyE", input_key_t::E },
    { "KeyQ", input_key_t::Q },
	{ "KeyR", input_key_t::R },
	{ "KeyV", input_key_t::V },
    { "Escape", input_key_t::ESC },
    { "Space", input_key_t::SPACE },
    { "ShiftLeft", input_key_t::LSHIFT }
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

typedef void ( view::camera_t::*camKeyFunc_t )( input_key_t key );

template< camKeyFunc_t cameraKeyFunc >
INLINE EM_BOOL KeyInputFunc( int32_t eventType, const EmscriptenKeyboardEvent* keyEvent, void* userData )
{
    UNUSEDPARAM( userData );

    printf( "EMCode: %s, EMKey: %s\n", keyEvent->code, keyEvent->key );

    app_t& app = app_t::GetInstance();

    auto entry = emKeyMap.find( keyEvent->code );

    if ( entry == emKeyMap.end() )
    {
        return 0;
    }

	switch ( entry->second )
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
			CALL_MEM_FNPTR( app.camera, cameraKeyFunc )( entry->second );
			break;
	}

    return 1;
}

EM_BOOL MouseMoveFunc( int32_t eventType, const EmscriptenMouseEvent* mouseEvent, void* userData )
{
    UNUSEDPARAM( eventType );
    UNUSEDPARAM( userData );

    printf( "Mouse { x: %ld, y: %ld\n }", mouseEvent->canvasX, mouseEvent->canvasY );

    app_t& app = app_t::GetInstance();

    EmscriptenPointerlockChangeEvent pl;
    emscripten_get_pointerlock_status( &pl );

    bool activePl = !!pl.isActive;

    if ( activePl )
    {
        app.camera.EvalMouseMove( ( float ) mouseEvent->movementX, ( float ) mouseEvent->movementY, false );
    }

    return 1;
}
#endif // EMSCRIPTEN

app_t::app_t( uint32_t width_ , uint32_t height_ )
	: width( width_ ),
	  height( height_ ),
	  globalVao( 0 )
{
	SDL_Init( SDL_INIT_VIDEO );
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, OP_GL_MAJOR_VERSION );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, OP_GL_MINOR_VERSION );

#ifndef OP_GL_USE_ES
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 24 );
	SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
#endif

	SDL_CreateWindowAndRenderer( width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN, &window, &renderer );
	context = SDL_GL_CreateContext( window );

	if ( !context )
	{
		MLOG_ERROR( "SDL_Error: %s", SDL_GetError() );
		return;
	}

#ifndef OP_GL_USE_ES
	glewExperimental = true;
	GLenum glewSuccess = glewInit();
	if ( glewSuccess != GLEW_OK )
	{
		MLOG_ERROR( "Could not initialize GLEW: %s", ( const char* ) glewGetErrorString( glewSuccess ) );
	}
#endif

	SDL_RendererInfo info;
	SDL_GetRendererInfo( renderer, &info );

	GL_CHECK( glClearColor( 0.0f, 0.0f, 0.0f, 1.0f ) );

	SDL_RenderPresent( renderer );

	pipeline.reset( new rend::pipeline_t() );

	const rend::shader_program_t& program = pipeline->programs[ "single_color" ];

	program.Bind();

	camera.SetViewOrigin( glm::vec3( 0.0f, 0.0f, 5.0f ) );
	camera.SetPerspective( 60.0f, ( float ) width, ( float ) height, 0.1f, 10000.0f );
	program.LoadMat4( "viewToClip", camera.GetViewParams().clipTransform );

	program.Release();

	GL_CHECK( glDisable( GL_CULL_FACE ) );
	GL_CHECK( glEnable( GL_DEPTH_TEST ) );
	GL_CHECK( glDepthFunc( GL_LEQUAL ) );
	GL_CHECK( glClearDepthf( 1.0f ) );

	GL_CHECK( glEnable( GL_BLEND ) );
	GL_CHECK( glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ) );

	gen.reset( new map::generator_t() );

	running = true;
}

app_t::~app_t( void )
{
	SDL_Quit();
}

app_t& app_t::GetInstance( void )
{
	static app_t app( 1366, 768 );
	return app;
}

void app_t::ResetMap( void )
{
	gen.reset( new map::generator_t() );
}

void app_t::ToggleCulling( void )
{
	drawAll = !drawAll;
}

void Draw_Group( const app_t& app,
				 const view::params_t& vp,
				 const std::vector< const map::tile_t* >& billboards,
				 const std::vector< const map::tile_t* >& walls )
{
	const rend::shader_program_t& singleColor = app.pipeline->programs.at( "single_color" );
	const rend::shader_program_t& billboard = app.pipeline->programs.at( "billboard" );

	const rend::draw_buffer_t& coloredCube = app.pipeline->drawBuffers.at( "colored_cube" );
	const rend::draw_buffer_t& billboardBuffer = app.pipeline->drawBuffers.at( "billboard" );

	for ( const map::tile_t* tile: walls )
	{
		singleColor.Bind();
		singleColor.LoadVec4( "color", glm::vec4( 0.5f, 0.5f, 0.5f, 1.0f ) );
		singleColor.LoadMat4( "modelToView", vp.transform * tile->bounds->transform );

		coloredCube.Render( singleColor );

		singleColor.Release();
	}

	for ( const map::tile_t* tile: billboards )
	{
		billboard.Bind();

		billboard.LoadMat4( "modelToView", vp.transform );
		billboard.LoadMat4( "viewToClip", vp.clipTransform );
		billboard.LoadVec3( "origin", glm::vec3( tile->bounds->transform[ 3 ] ) );
		billboard.LoadMat3( "viewOrient", glm::mat3( vp.inverseOrient ) );

		app.gen->billTexture.Bind( 0, "image", billboard );
		billboardBuffer.Render( billboard );
		app.gen->billTexture.Release( 0 );

		billboard.Release();
	}

}

void App_Frame( void )
{
    app_t& app = app_t::GetInstance();

#ifdef EMSCRIPTEN
    if ( !app.running )
    {
        emscripten_cancel_main_loop();
        std::exit( 0 );
    }
#endif

    app.camera.Update();

	const view::params_t& vp = app.camera.GetViewParams();

	app.frustum.Update( vp );

	GL_CHECK( glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ) );

	if ( app.drawAll )
	{
		Draw_Group( app, vp, app.gen->billboards, app.gen->walls );
	}
	else
	{
		std::vector< const map::tile_t* > billboards, walls;
		app.gen->GetEntities( billboards, walls, app.frustum, vp );
		Draw_Group( app, vp, billboards, walls );
	}
}

// temporary hack to get around the fact that querying for an app
// instance if an error is happening causes problems; we use this flag the exit.
static bool* runningPtr = nullptr;

uint32_t App_Exec( void )
{   
    app_t& app = app_t::GetInstance();
	runningPtr = &app.running;

#ifdef EMSCRIPTEN
    int32_t ret;
    SET_CALLBACK_RESULT( emscripten_set_keydown_callback( nullptr, nullptr, 0, ( em_key_callback_func )&KeyInputFunc< &view::camera_t::EvalKeyPress > ) );
    SET_CALLBACK_RESULT( emscripten_set_keyup_callback( nullptr, nullptr, 0, ( em_key_callback_func )&KeyInputFunc< &view::camera_t::EvalKeyRelease > ) );
    SET_CALLBACK_RESULT( emscripten_set_mousemove_callback( "#canvas", nullptr, 1, ( em_mouse_callback_func )&MouseMoveFunc ) );

    emscripten_set_main_loop( ( em_callback_func )&App_Frame, 0, 1 );
#else
    while ( app.running )
    {
        App_Frame();

        SDL_GL_SwapWindow( app.window );

        SDL_Event e;
        while ( SDL_PollEvent( &e ) )
        {
            switch ( e.type )
            {
                case SDL_KEYDOWN:
                    switch ( e.key.keysym.sym )
                    {
                        case SDLK_ESCAPE:
							SDL_SetRelativeMouseMode( SDL_FALSE );
                            app.running = false;
                            break;
                        case SDLK_F1:
                            app.mouseShown = !app.mouseShown;
                            if ( app.mouseShown )
                            {
                                SDL_SetRelativeMouseMode( SDL_FALSE );
                            }
                            else
                            {
                                SDL_SetRelativeMouseMode( SDL_TRUE );
                            }
                            break;
						case SDLK_r:
							app.ResetMap();
							break;
						case SDLK_v:
							app.ToggleCulling();
							break;

                        default:
                            app.camera.EvalKeyPress( ( input_key_t ) e.key.keysym.sym );
                            break;

                    }
                    break;
                case SDL_KEYUP:
                    app.camera.EvalKeyRelease( ( input_key_t ) e.key.keysym.sym );
                    break;
                case SDL_MOUSEMOTION:
                    if ( !app.mouseShown )
                    {
                        app.camera.EvalMouseMove( ( float ) e.motion.xrel, ( float ) e.motion.yrel, false );
                    }
                    break;
            }
        }
    }
#endif

    return 0;
}

}

void FlagExit( void )
{
	*runningPtr = false;
}

// SDL2 defines main as a macro for some reason on Windows
#ifdef _WIN32
#	undef main
#endif

int main( void ) 
{
    return App_Exec();
}

