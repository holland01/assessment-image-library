#include "game.h"

#include <iostream>

#include "renderer.h"

#ifdef OP_UNIX
	#include <unistd.h>
#endif

#include <stdint.h>
#include <array>
#include <cstdlib>
#include <unordered_map>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_access.hpp>

void Game_Frame( void );

#include "eminput.h"

void Draw_Group( game_t& game,
                 const view_params_t& vp,
                 billboard_list_t& billboards,
                 wall_list_t& walls,
                 freespace_list_t& freeSpace );

game_t::game_t( uint32_t width_ , uint32_t height_ )
	: width( width_ ),
	  height( height_ ),
	  frameTime( 0.0f ),
	  lastTime( 0.0f ),
	  startTime( 0.0f ),
	  camera( nullptr ),
	  drawBounds( nullptr ),
	  world( 1.0f, 1.0f / 30.0f )
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

	pipeline.reset( new pipeline_t() );

	const shader_program_t& program = pipeline->programs[ "single_color" ];
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
	//GL_CHECK( glEnable( GL_BLEND ) );
	//GL_CHECK( glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ) );

#ifndef OP_GL_USE_ES
	GL_CHECK( glPointSize( 10.0f ) );
#endif

	groundPlane.normal = glm::vec3( 0.0f, 1.0f, 0.0f );
	groundPlane.d = 0.0f;

    gen.reset( new generator_t() );

	std::sort( gen->freeSpace.begin(), gen->freeSpace.end(), []( const tile_t* a, const tile_t* b ) -> bool
	{
		glm::vec2 va( a->x, a->z ), vb( b->x, b->z );

		return glm::length( va ) < glm::length( vb );
	});

	const tile_t* tile = gen->freeSpace[ gen->freeSpace.size() / 2 ];

	{
        body_t* body = new body_t( body_t::RESET_VELOCITY_BIT | body_t::RESET_FORCE_ACCUM_BIT );
        body->SetPosition( glm::vec3( tile->x, 0.0f, tile->z ) );
        body->SetMass( 80.0f );

        player.body.reset( body );
        world.bodies.push_back( player.body );
	}
	{
        body_t* specBody = new body_t( body_t::RESET_VELOCITY_BIT | body_t::RESET_FORCE_ACCUM_BIT );
        specBody->SetPosition( glm::vec3( tile->x, 10.0f, tile->z ) );
        specBody->SetMass( 5.0f );

		spec.mode = input_client_t::MODE_SPEC;
        spec.body.reset( specBody );
        spec.Sync();

        world.bodies.push_back( spec.body );
	}
	camera = &player;
    drawBounds = spec.bounds.get();

	running = true;
}

game_t::~game_t( void )
{
	SDL_Quit();
}

game_t& game_t::GetInstance( void )
{
	static game_t game( 1366, 768 );
	return game;
}

void game_t::ResetMap( void )
{
    gen.reset( new generator_t() );
}

void game_t::ToggleCulling( void )
{
	drawAll = !drawAll;
}

void game_t::Tick( void )
{
	frameTime = startTime - lastTime;
}

void game_t::Draw( void )
{
    const view_params_t& vp = camera->GetViewParams();

	GL_CHECK( glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ) );

	if ( drawAll )
	{
		Draw_Group( *this, vp, gen->billboards, gen->walls, gen->freeSpace );
	}
	else
	{
		Draw_Group( *this, vp, billboards, walls, freeSpace );
	}

    // Clear depth buffer so the reticule renders over anything else it tests
    // against by default
    GL_CHECK( glClear( GL_DEPTH_BUFFER_BIT ) );

    const shader_program_t& ssSingleColor = pipeline->programs.at( "single_color_ss" );

    ssSingleColor.Bind();
    ssSingleColor.LoadVec4( "color", glm::vec4( 1.0f ) );

    imm_draw_t drawer( ssSingleColor );
    drawer.Begin( GL_POINTS );
    drawer.Vertex( glm::vec3( 0.0f, 0.0f, 0.0f ) );
    drawer.End();

    ssSingleColor.Release();
}

void Draw_Group( game_t& game,
                 const view_params_t& vp,
                 billboard_list_t& billboards,
                 wall_list_t& walls,
                 freespace_list_t& freeSpace )
{

    // Programs from the global pipeline...
	const shader_program_t& singleColor = game.pipeline->programs.at( "single_color" );
	const shader_program_t& billboard = game.pipeline->programs.at( "billboard" );

	const draw_buffer_t& coloredCube = game.pipeline->drawBuffers.at( "colored_cube" );
	const draw_buffer_t& billboardBuffer = game.pipeline->drawBuffers.at( "billboard" );

    // immDrawer can be used in some arbitrary code block that is aware of the renderer to draw something.
    // It's useful for debugging...
	imm_draw_t drawer( singleColor );
	immDrawer = &drawer;

    // For empty tiles we care about...
	glm::mat4 quadTransform( glm::rotate( glm::mat4( 1.0f ), glm::half_pi< float >(), glm::vec3( 1.0f, 0.0f, 0.0f ) ) );
	quadTransform[ 3 ].y = -1.0f;

    // Some lambda goodness
	auto LDrawQuad = [ &vp, &quadTransform, &billboardBuffer, &singleColor ]( const glm::mat4& transform,
			const glm::vec3& color )
	{
		singleColor.LoadVec4( "color", glm::vec4( color, 1.0f ) );
		singleColor.LoadMat4( "modelToView", vp.transform * transform * quadTransform );
		billboardBuffer.Render( singleColor );
	};

	auto LDrawBounds = [ &vp, &singleColor, &coloredCube ]( const bounding_box_t& bounds, const glm::vec3& color )
	{
		singleColor.LoadVec4( "color", glm::vec4( color, 1.0f ) );
        singleColor.LoadMat4( "modelToView",  vp.transform * bounds.GetTransform() );
		coloredCube.Render( singleColor );
	};

    auto LApplyForce = [ &game ]( const glm::vec3& normal, const body_t& body )
    {
        if ( game.camera->body )
        {
            game.camera->body->ApplyForce( P_GenericCollideNormal( normal, body, *( game.camera->body ) ) );
        }
    };

    // Load a grey color so it looks somewhat fancy
	singleColor.Bind();
	singleColor.LoadVec4( "color", glm::vec4( 0.5f, 0.5f, 0.5f, 1.0f ) );
	for ( const tile_t* tile: walls )
	{
        LDrawBounds( *( tile->bounds ), glm::vec3( 0.5f ) );

		// don't test collision unless distance from player to object is within a certain range
        if ( glm::distance( glm::vec3( ( *tile->bounds )[ 3 ] ), vp.origin ) <= 2.0f )
		{
			half_space_t hs;
			glm::vec3 normal;
            LDrawBounds( *( tile->bounds ), glm::vec3( 0.0f, 1.0f, 0.0f ) );
            if ( game.gen->CollidesWall( normal, *tile, *game.camera->bounds, hs ) )
			{
                LApplyForce( normal, *( tile->body ) );
			}
		}
	}

    if ( game.bullet )
    {
        LDrawBounds( *( game.bullet->bounds ), glm::vec3( 1.0f, 0.0f, 0.0f ) );
    }

    // Draw the bounds of the camera not currently being used
	LDrawBounds( *( game.drawBounds ), glm::vec3( 1.0f, 0.0f, 1.0f ) );

	singleColor.Release();

    // Draw all billboards
	billboard.Bind();
	billboard.LoadMat4( "modelToView", vp.transform );
	billboard.LoadMat4( "viewToClip", vp.clipTransform );

    // This load ensures that the billboard is always facing the viewer
	billboard.LoadMat3( "viewOrient", glm::mat3( vp.inverseOrient ) );
    for ( tile_t* tile: billboards )
	{
        billboard.LoadVec3( "origin", glm::vec3( ( *( tile->bounds ) )[ 3 ] ) );

        // Set orientation so collisions are properly computed regardless of direction
        tile->body->SetOrientation( vp.inverseOrient );
        tile->Sync();

        // Check for an intersection...
		glm::vec3 normal;
        if ( game.camera->bounds->IntersectsBounds( normal, *( tile->bounds ) ) )
		{
            LApplyForce( normal, *( tile->body ) );
		}

        if ( game.bullet )
        {
            glm::vec3 normal;
            if ( game.bullet->bounds->IntersectsBounds( normal, *( tile->bounds ) ) )
            {
                tile->SetColor( glm::vec3( 1.0f, 0.0f, 0.0f ) );
                game.bullet.release();
            }
        }


        billboard.LoadVec4( "color", tile->GetColor() );

        game.gen->billTexture.Bind( 0, "image", billboard );
		billboardBuffer.Render( billboard );

		game.gen->billTexture.Release( 0 );
	}
	billboard.Release();

    // Mark all free spaces here
    singleColor.Bind();
	for ( const tile_t* tile: freeSpace )
	{
        LDrawQuad( tile->bounds->GetTransform(), glm::vec3( 0.0f, 0.0f, 0.5f ) );
	}
	singleColor.Release();
}

void Game_Frame( void )
{
	game_t& game = game_t::GetInstance();

	game.startTime = GetTime();

#ifdef EMSCRIPTEN
	if ( !game.running )
    {
        emscripten_cancel_main_loop();
        std::exit( 0 );
    }
#endif

    const view_params_t& vp = game.camera->GetViewParams();

	game.frustum.Update( vp );

	if ( !game.drawAll )
	{
		game.gen->GetEntities( game.billboards, game.walls, game.freeSpace, game.frustum, vp );
	}

	game.world.Update( game );
	game.Draw();

    // clear bodies which are added in world.Update call,
    // since we only want to integrate bodies which are in view
    game.world.bodies.clear();

	game.world.time = GetTime() - game.startTime;

    printf( "DT: %f, MoveStep: %f, FPS: %f" OP_CARRIAGE_RETURN, game.world.time, game.camera->viewParams.moveStep, 1.0f / game.world.time );
}

// temporary hack to get around the fact that querying for an game
// instance if an error is hgameening causes problems; we use this flag the exit.
static bool* runningPtr = nullptr;

uint32_t Game_Exec( void )
{   	
	game_t& game = game_t::GetInstance();
	runningPtr = &game.running;

#ifdef EMSCRIPTEN
	InitEmInput();
#else
	while ( game.running )
    {
		Game_Frame();

		SDL_GL_SwapWindow( game.window );

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
							game.running = false;
                            break;
                        case SDLK_F1:
							game.mouseShown = !game.mouseShown;
							if ( game.mouseShown )
                            {
                                SDL_SetRelativeMouseMode( SDL_FALSE );
                            }
                            else
                            {
                                SDL_SetRelativeMouseMode( SDL_TRUE );
                            }
                            break;
						case SDLK_r:
							game.ResetMap();
							break;
						case SDLK_v:
							game.ToggleCulling();
							break;
						case SDLK_c:
							if ( game.camera == &game.player )
							{
                                game.drawBounds = game.player.bounds.get();
								game.camera = &game.spec;
							}
							else
							{
                                game.drawBounds = game.spec.bounds.get();
								game.camera = &game.player;
							}
							break;

                        default:
							game.camera->EvalKeyPress( ( input_key_t ) e.key.keysym.sym );
                            break;


                    }
                    break;
                case SDL_KEYUP:
					game.camera->EvalKeyRelease( ( input_key_t ) e.key.keysym.sym );
                    break;
                case SDL_MOUSEMOTION:
					if ( !game.mouseShown )
                    {
						game.camera->EvalMouseMove( ( float ) e.motion.xrel, ( float ) e.motion.yrel, false );
                    }
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    if ( e.button.button == SDL_BUTTON_LEFT )
                    {
                        // shoot weapon. Do hit testing first by casting a projectile body
                        // out into the stratosphere. If its bounds collides with
                        // one of the billboards, color the billboard red or something.

                        if ( game.camera->body )
                        {
                            game.bullet.reset( new entity_t( entity_t::BODY_DEPENDENT, new bounding_box_t, new body_t ) );
                            game.bullet->body->SetOrientation( game.camera->body->GetOrientation() );
                            game.bullet->body->ApplyVelocity( glm::vec3( 0.0f, 0.0f, -1.0f ) );
                            game.bullet->body->SetPosition( game.camera->GetViewParams().origin );
                        }
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
	return ( float )SDL_GetTicks() * 0.001f;
}

// SDL2 defines main as a macro for some reason on Windows
#ifdef _WIN32
#	undef main
#endif

int main( void ) 
{
	return Game_Exec();
}

