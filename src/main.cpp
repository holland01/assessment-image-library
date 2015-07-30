//#include "def.h"

/*
#ifndef EMSCRIPTEN
#   define EMSCRIPTEN
#endif
*/

#include <iostream>
#ifdef EMSCRIPTEN
#	include "eminput.cpp"
#endif

#include "renderer.h"
#include "geom.h"
#include "input.h"
#include "map.h"
#include "physics.h"

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

struct game_t
{
	bool running = false, mouseShown = true, drawAll = false;
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_GLContext context = nullptr;

    uint32_t width, height;

	float frameTime, lastTime, startTime;

	std::unique_ptr< map::generator_t > gen;
	std::unique_ptr< rend::pipeline_t > pipeline;

	geom::plane_t groundPlane;

	input_client_t player, spec;
	input_client_t* camera;

	geom::bounding_box_t* drawBounds;

	view::frustum_t	frustum;

	game_t( uint32_t width, uint32_t height );
   ~game_t( void );

	void ResetMap( void );
	void ToggleCulling( void );
	void Tick( void );

	static game_t& GetInstance( void );
};

game_t::game_t( uint32_t width_ , uint32_t height_ )
	: width( width_ ),
	  height( height_ ),
	  frameTime( 0.0f ),
	  lastTime( 0.0f ),
	  startTime( 0.0f ),
	  camera( nullptr )
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

	spec.SetPerspective( 60.0f, ( float ) width, ( float ) height, 0.1f, 10000.0f );
	player.SetPerspective( 60.0f, ( float ) width, ( float ) height, 0.1f, 10000.0f );

	program.LoadMat4( "viewToClip", player.GetViewParams().clipTransform );
	program.Release();

	GL_CHECK( glEnable( GL_TEXTURE_2D ) );
	GL_CHECK( glDisable( GL_CULL_FACE ) );
	GL_CHECK( glEnable( GL_DEPTH_TEST ) );
	GL_CHECK( glDepthFunc( GL_LEQUAL ) );
	GL_CHECK( glClearDepthf( 1.0f ) );
	GL_CHECK( glEnable( GL_BLEND ) );
	GL_CHECK( glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ) );

#ifndef OP_GL_USE_ES
	GL_CHECK( glPointSize( 3.0f ) );
#endif

	gen.reset( new map::generator_t() );

	std::sort( gen->freeSpace.begin(), gen->freeSpace.end(), []( const map::tile_t* a, const map::tile_t* b ) -> bool
	{
		glm::vec2 va( a->x, a->z ), vb( b->x, b->z );

		return glm::length( va ) < glm::length( vb );
	});
	const map::tile_t* tile = gen->freeSpace[ gen->freeSpace.size() / 2 ];
	player.body.position = glm::vec3( tile->x, 0.0f, tile->z );

	player.body.initialForce = glm::vec3( 0.0f, -9.8, 0.0f );
	player.body.Reset();
	player.body.invMass = 1.0f / 100.0f;

	groundPlane.normal = glm::vec3( 0.0f, 1.0f, 0.0f );
	groundPlane.d = 0.0f;

	spec.mode = input_client_t::MODE_SPEC;
	spec.viewParams.origin = glm::vec3( tile->x, 10.0f, tile->z );
	//spec.body.invMass = 1.0f / 10.0f;
	spec.Update( 1000.0f );

	camera = &player;
	drawBounds = &spec.bounds;

	running = true;
}

game_t::~game_t( void )
{
	SDL_Quit();
}

game_t& game_t::GetInstance( void )
{
	static game_t app( 1366, 768 );
	return app;
}

void game_t::ResetMap( void )
{
	gen.reset( new map::generator_t() );
}

void game_t::ToggleCulling( void )
{
	drawAll = !drawAll;
}

void game_t::Tick( void )
{
	frameTime = startTime - lastTime;
}

/*
namespace {

std::array< glm::vec4, 4 > colors =
{{
	glm::vec4( 1.0f, 0.0f, 0.0f, 1.0f ),
	glm::vec4( 0.0f, 1.0f, 0.0f, 1.0f ),
	glm::vec4( 0.0f, 0.0f, 1.0f, 1.0f ),
	glm::vec4( 1.0f, 1.0f, 0.0f, 1.0f )
}};

uint32_t yesCount = 0;

}
*/

void Draw_Group( game_t& app,
				 const view::params_t& vp,
				 const std::vector< const map::tile_t* >& billboards,
				 const std::vector< const map::tile_t* >& walls,
				 const std::vector< const map::tile_t* >& freeSpace )
{

	const rend::shader_program_t& singleColor = app.pipeline->programs.at( "single_color" );
	const rend::shader_program_t& billboard = app.pipeline->programs.at( "billboard" );

	const rend::draw_buffer_t& coloredCube = app.pipeline->drawBuffers.at( "colored_cube" );
	const rend::draw_buffer_t& billboardBuffer = app.pipeline->drawBuffers.at( "billboard" );

	//rend::imm_draw_t drawer( singleColor );

	glm::mat4 quadTransform( glm::rotate( glm::mat4( 1.0f ), glm::half_pi< float >(), glm::vec3( 1.0f, 0.0f, 0.0f ) ) );

	quadTransform[ 3 ].y = -1.0f;

	auto LDrawQuad = [ &vp, &quadTransform, &billboardBuffer, &singleColor ]( const glm::mat4& transform,
			const glm::vec3& color )
	{
		singleColor.LoadVec4( "color", glm::vec4( color, 1.0f ) );
		singleColor.LoadMat4( "modelToView", vp.transform * transform * quadTransform );
		billboardBuffer.Render( singleColor );
	};

	auto LDrawBounds = [ &vp, &singleColor, &coloredCube ]( const geom::bounding_box_t& bounds, const glm::vec3& color )
	{
		singleColor.LoadVec4( "color", glm::vec4( color, 1.0f ) );
		singleColor.LoadMat4( "modelToView",  vp.transform * bounds.transform );
		coloredCube.Render( singleColor );
	};

	singleColor.Bind();
	singleColor.LoadVec4( "color", glm::vec4( 0.5f, 0.5f, 0.5f, 1.0f ) );
	for ( const map::tile_t* tile: walls )
	{
		LDrawBounds( *( tile->bounds ), glm::vec3( 0.5f ) );

		{
			geom::half_space_t hs;
			if ( app.gen->CollidesWall( *tile, app.camera->bounds, hs ) )
			{
				//app.camera.body.forceAccum += hs.extents[ 2 ] * 0.1f;
			}
		}
	}

	LDrawBounds( *( app.drawBounds ), glm::vec3( 1.0f, 0.0f, 1.0f ) );

	singleColor.Release();

	billboard.Bind();
	billboard.LoadMat4( "modelToView", vp.transform );
	billboard.LoadMat4( "viewToClip", vp.clipTransform );
	billboard.LoadMat3( "viewOrient", glm::mat3( vp.inverseOrient ) );
	for ( const map::tile_t* tile: billboards )
	{
		billboard.LoadVec3( "origin", glm::vec3( tile->bounds->transform[ 3 ] ) );

		app.gen->billTexture.Bind( 0, "image", billboard );
		billboardBuffer.Render( billboard );
		app.gen->billTexture.Release( 0 );
	}
	billboard.Release();

	singleColor.Bind();
	for ( const map::tile_t* tile: freeSpace )
	{
		LDrawQuad( tile->bounds->transform, glm::vec3( 0.0f, 0.0f, 0.5f ) );
	}
	singleColor.Release();
}

namespace  {
	bool PointPlanePredicate( float value )
	{
		return value <= 0.0f;
	}
}

void App_Frame( void )
{
	game_t& app = game_t::GetInstance();

	app.startTime = GetTime();

#ifdef EMSCRIPTEN
    if ( !app.running )
    {
        emscripten_cancel_main_loop();
        std::exit( 0 );
    }
#endif

	const view::params_t& vp = app.camera->GetViewParams();

	{
		app.camera->Update( app.frameTime );
		std::array< glm::vec3, 8 > clipBounds;
		app.camera->bounds.GetPoints( clipBounds );

		if ( geom::PointPlaneTest< 8, PointPlanePredicate >( clipBounds, app.groundPlane ) )
		{
			app.camera->body.initialForce.y = 0.0f;
		}

		app.frustum.Update( vp );
	}

	GL_CHECK( glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ) );

	if ( app.drawAll )
	{
		Draw_Group( app, vp, app.gen->billboards, app.gen->walls, app.gen->freeSpace );
	}
	else
	{
		std::vector< const map::tile_t* > billboards, walls, freeSpace;
		app.gen->GetEntities( billboards, walls, freeSpace, app.frustum, vp );
		Draw_Group( app, vp, billboards, walls, freeSpace );
	}

	app.frameTime = GetTime() - app.startTime;
}

// temporary hack to get around the fact that querying for an app
// instance if an error is happening causes problems; we use this flag the exit.
static bool* runningPtr = nullptr;

uint32_t App_Exec( void )
{   	
	game_t& app = game_t::GetInstance();
	runningPtr = &app.running;

#ifdef EMSCRIPTEN
	InitEmInput();
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
						case SDLK_c:
							if ( app.camera == &app.player )
							{
								app.drawBounds = &app.player.bounds;
								app.camera = &app.spec;
							}
							else
							{
								app.drawBounds = &app.spec.bounds;
								app.camera = &app.player;
							}
							break;

                        default:
							app.camera->EvalKeyPress( ( input_key_t ) e.key.keysym.sym );
                            break;


                    }
                    break;
                case SDL_KEYUP:
					app.camera->EvalKeyRelease( ( input_key_t ) e.key.keysym.sym );
                    break;
                case SDL_MOUSEMOTION:
                    if ( !app.mouseShown )
                    {
						app.camera->EvalMouseMove( ( float ) e.motion.xrel, ( float ) e.motion.yrel, false );
                    }
                    break;
            }
        }
    }
#endif

    return 0;
}

void FlagExit( void )
{
	*runningPtr = false;
}

float GetTime( void )
{
	return ( float )SDL_GetTicks();
}

// SDL2 defines main as a macro for some reason on Windows
#ifdef _WIN32
#	undef main
#endif

int main( void ) 
{
    return App_Exec();
}

