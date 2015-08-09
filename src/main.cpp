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

namespace {
    quad_hierarchy_t::node_t::ptr_t testNode( nullptr );

    enum
    {
        DRAW_BILLBOARDS = 0x1,
        DRAW_HALFSPACES = 0x2
    };

    const float DISTANCE_THRESHOLD = 2.0f;
    uint32_t drawFlags = 0;
}

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
      world( 1.0f, OP_PHYSICS_DT )
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
    GL_CHECK( glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ) );

#ifndef OP_GL_USE_ES
	GL_CHECK( glPointSize( 10.0f ) );
#endif

	groundPlane.normal = glm::vec3( 0.0f, 1.0f, 0.0f );
	groundPlane.d = 0.0f;

    gen.reset( new tile_generator_t() );

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

    glm::mat4 s( glm::scale( glm::mat4( 1.0f ), glm::vec3( ( float ) tile_generator_t::GRID_SIZE, 1.0f, ( float ) tile_generator_t::GRID_SIZE ) ) );
    glm::mat4 t( glm::translate( glm::mat4( 1.0f ), glm::vec3( ( float ) tile_generator_t::GRID_SIZE, 0.0f, ( float )tile_generator_t::GRID_SIZE ) ) );

    glm::mat4 boundsT( t * s );

    //testNode.reset( new quad_hierarchy_t::node_t( 0, 3, std::move( bounding_box_t( boundsT ) ) ) );

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
    gen.reset( new tile_generator_t() );
}

void game_t::ToggleCulling( void )
{
	drawAll = !drawAll;

    if ( !drawAll )
    {
        camera->viewParams.moveStep = OP_DEFAULT_MOVE_STEP;
    }
    else
    {
        camera->viewParams.moveStep = OP_DEFAULT_MOVE_STEP * 100.0f;
    }
}

void game_t::Tick( void )
{
	frameTime = startTime - lastTime;
}

void game_t::Draw( void )
{
    const view_params_t& vp = camera->GetViewParams();

	GL_CHECK( glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ) );

    if ( testNode )
    {
        testNode->Draw( *pipeline, vp );
    }

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


    {
        const shader_program_t& ssSingleColor = pipeline->programs.at( "single_color_ss" );

        ssSingleColor.Bind();
        ssSingleColor.LoadVec4( "color", glm::vec4( 1.0f ) );

        imm_draw_t drawer( ssSingleColor );

        // Draw reticule
        drawer.Begin( GL_POINTS );
        drawer.Vertex( glm::vec3( 0.0f ) );
        drawer.End();

        ssSingleColor.Release();
    }

    {
        const shader_program_t& singleColor = pipeline->programs.at( "single_color" );

        singleColor.Bind();
        singleColor.LoadMat4( "modelToView", vp.transform * glm::translate( glm::mat4( 1.0f ), glm::vec3( 0.0f, 1.0f, 0.0f ) ) );
        singleColor.LoadVec4( "color", glm::vec4( 0.0f, 1.0f, 0.0f, 1.0f ) );

        imm_draw_t drawer( singleColor );

        const float AXIS_SIZE = 100.0f;

        // Draw coordinate space axes
        drawer.Begin( GL_LINES );
        drawer.Vertex( glm::vec3( 0.0f ) );
        drawer.Vertex( glm::vec3( 0.0f, 0.0f, AXIS_SIZE ) );
        drawer.Vertex( glm::vec3( 0.0f ) );
        drawer.Vertex( glm::vec3( 0.0f, AXIS_SIZE, 0.0f ) );
        drawer.Vertex( glm::vec3( 0.0f ) );
        drawer.Vertex( glm::vec3( AXIS_SIZE, 0.0f, 0.0f ) );
        drawer.End();

        singleColor.Release();
    }

}

void game_t::FireGun( void )
{
    if ( camera->body )
    {
        bullet.reset( new entity_t( entity_t::BODY_DEPENDENT, new bounding_box_t, new body_t ) );
        bullet->body->SetOrientation( glm::mat3( 0.1f ) * camera->body->GetOrientation() );
        bullet->body->ApplyVelocity( glm::vec3( 0.0f, 0.0f, -10.0f ) ); // Compensate for the applied scale of the bounds
        bullet->body->SetPosition( camera->GetViewParams().origin );
    }
}

void Draw_Bounds( game_t& game, const bounding_box_t& bounds, const glm::vec3& color, float alpha = 1.0f )
{
    const shader_program_t& singleColor = game.pipeline->programs.at( "single_color" );
    const draw_buffer_t& coloredCube = game.pipeline->drawBuffers.at( "colored_cube" );
    const view_params_t& vp = game.camera->GetViewParams();

    singleColor.LoadVec4( "color", glm::vec4( color, alpha ) );
    singleColor.LoadMat4( "modelToView",  vp.transform * bounds.GetTransform() );
    coloredCube.Render( singleColor );
}

void Apply_Force( game_t& game, const glm::vec3& normal, const body_t& body )
{
    if ( game.camera->body )
    {
       game.camera->body->ApplyForce( P_GenericCollideNormal( normal, body, *( game.camera->body ) ) );
    }
}

void Draw_Billboards( game_t& game, const view_params_t& vp, billboard_list_t& billboards )
{
    const shader_program_t& singleColor = game.pipeline->programs.at( "single_color" );
    const shader_program_t& billboard = game.pipeline->programs.at( "billboard" );
    const draw_buffer_t& billboardBuffer = game.pipeline->drawBuffers.at( "billboard" );

    load_blend_t blend( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    // Draw all billboards
    billboard.Bind();
    billboard.LoadMat4( "modelToView", vp.transform );
    billboard.LoadMat4( "viewToClip", vp.clipTransform );

    imm_draw_t drawer( singleColor );

    for ( tile_t* tile: billboards )
    {
        glm::vec3 boundsOrigin( ( *( tile->bounds ) )[ 3 ] );

        billboard.LoadVec3( "origin", boundsOrigin );

        glm::vec3 dirToCam( vp.origin - boundsOrigin );
        dirToCam.y = 0.0f;
        dirToCam = glm::normalize( dirToCam );

        glm::mat3 orient( G_OrientByDirection( dirToCam, glm::vec3( 0.0f, 0.0f, 1.0f ), glm::vec3( -1.0f, 0.0f, 0.0f ) ) );

        // This load ensures that the billboard is always facing the viewer
        billboard.LoadMat3( "viewOrient", orient );

        // Set orientation so collisions are properly computed regardless of direction
        tile->body->SetOrientation( orient );
        tile->Sync();

        if ( glm::distance( tile->body->GetPosition(), vp.origin ) <= DISTANCE_THRESHOLD )
        {
            // Check for an intersection...
            glm::vec3 normal;
            if ( game.camera->bounds->IntersectsBounds( normal, *( tile->bounds ) ) )
            {
                Apply_Force( game, normal, *( tile->body ) );
            }
        }

        if ( game.bullet )
        {
            glm::vec3 normal;

            // Scale the initial bounds by a smaller size, since it will make hitting
            // the enemies more challenging and not cause issues with enemies
            // which are near each other with their overlapping bounds.
            glm::mat4 s( glm::scale( glm::mat4( 1.0f ), glm::vec3( 0.4f ) ) );
            bounding_box_t bounds( tile->bounds->GetTransform() * s );

            if ( game.bullet->bounds->IntersectsBounds( normal, bounds ) )
            {
                tile->SetColor( glm::vec3( 1.0f, 0.0f, 0.0f ) );
                game.bullet.release();
            }
        }

        billboard.LoadVec4( "color", tile->GetColor() );

        game.gen->billTexture.Bind( 0, "image", billboard );
        billboardBuffer.Render( billboard );
        game.gen->billTexture.Release( 0 );

        singleColor.Bind();
        Draw_Bounds( game, *( tile->bounds ), glm::vec3( 0.5f ), 0.5f );

        singleColor.LoadVec4( "color", glm::vec4( 1.0f, 0.0f, 0.0f, 1.0f ) );
        singleColor.LoadMat4( "modelToView", vp.transform * tile->bounds->GetTransform() );
        drawer.Begin( GL_LINES );
        drawer.Vertex( glm::vec3( 0.0f ) );
        drawer.Vertex( glm::vec3( 0.0f, 0.0f, 3.0f ) );
        drawer.End();

        billboard.Bind();
    }
    billboard.Release();
}

void Draw_Group( game_t& game,
                 const view_params_t& vp,
                 billboard_list_t& billboards,
                 wall_list_t& walls,
                 freespace_list_t& freeSpace )
{
    UNUSEDPARAM( freeSpace );

	const shader_program_t& singleColor = game.pipeline->programs.at( "single_color" );
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

    // Load a grey color so it looks somewhat fancy
	singleColor.Bind();
	singleColor.LoadVec4( "color", glm::vec4( 0.5f, 0.5f, 0.5f, 1.0f ) );
	for ( const tile_t* tile: walls )
	{
        Draw_Bounds( game, *( tile->bounds ), glm::vec3( 0.5f ) );

		// don't test collision unless distance from player to object is within a certain range
        if ( glm::distance( glm::vec3( ( *tile->bounds )[ 3 ] ), vp.origin ) <= DISTANCE_THRESHOLD )
		{
			half_space_t hs;
			glm::vec3 normal;

            if ( game.gen->CollidesWall( normal, *tile, *game.camera->bounds, hs ) )
			{
                Draw_Bounds( game, *( tile->bounds ), glm::vec3( 0.0f, 1.0f, 0.0f ) );
                Apply_Force( game, normal, *( tile->body ) );
			}
		}
	}

    if ( game.bullet )
    {
        Draw_Bounds( game, *( game.bullet->bounds ), glm::vec3( 1.0f, 0.0f, 0.0f ) );
    }

    if ( drawFlags & DRAW_BILLBOARDS )
    {
        Draw_Billboards( game, vp, billboards );
    }

    singleColor.Bind();
    singleColor.LoadMat4( "modelToView", vp.transform );
    singleColor.LoadVec4( "color", glm::vec4( 0.0f, 1.0f, 0.0f, 1.0f ) );

    if ( drawFlags & DRAW_HALFSPACES )
    {
        for ( const tile_t* wall: walls )
        {
            if ( wall->halfSpaceIndex < 0 )
            {
                continue;
            }

            const tile_generator_t::half_space_table_t& table =
                    game.gen->halfSpaceTable[ wall->halfSpaceIndex ];

            for ( int32_t i: table )
            {
                if ( i < 0 )
                {
                    continue;
                }

                game.gen->halfSpaces[ i ].Draw( drawer );
            }
        }
    }

    // Draw the bounds of the camera not currently being used
    Draw_Bounds( game, *( game.drawBounds ), glm::vec3( 1.0f, 0.0f, 1.0f ) );

    // Mark all free spaces here
    singleColor.Bind();

    for ( const tile_region_t& region: game.gen->regions )
    {
        for ( const tile_t* tile: region.tiles )
        {
            LDrawQuad( tile->bounds->GetTransform(), glm::vec3( region.color ) );
        }
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

    //rintf( "DT: %f, measure count: %iu, MoveStep: %f, FPS: %f" OP_CARRIAGE_RETURN,
   //         game.world.time, game.world.lastMeasureCount, game.camera->viewParams.moveStep, 1.0f / game.world.time );

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

    printf( "FPS: %f\r", 1.0f / game.world.time );
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
                        case SDLK_b:
                            drawFlags ^= DRAW_BILLBOARDS;
                            break;

                        case SDLK_h:
                            drawFlags ^= DRAW_HALFSPACES;
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
                        game.FireGun();
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

