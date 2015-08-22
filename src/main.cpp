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
    enum
    {
        DRAW_BILLBOARDS = 1 << 0,
        DRAW_BILLBOARD_BOUNDS = 1 << 1,
        DRAW_HALFSPACES = 1 << 2,
        DRAW_REGIONS_ADJACENT = 1 << 3,
        DRAW_REGIONS_BOUNDS = 1 << 4,
        DRAW_WALLS = 1 << 5,
        DRAW_QUAD_REGIONS = 1 << 6
    };

    const float DISTANCE_THRESHOLD = 2.0f;

    std::unordered_map< std::string, uint32_t > gDrawTestConfig =
    {
        { "adjacency_test", DRAW_REGIONS_ADJACENT | DRAW_WALLS },
        { "default", DRAW_BILLBOARDS | DRAW_WALLS | DRAW_HALFSPACES | DRAW_BILLBOARD_BOUNDS },
        { "collision_test", DRAW_BILLBOARDS | DRAW_HALFSPACES },
        { "bounds_tiles_test", DRAW_REGIONS_BOUNDS }
    };

     uint32_t gDrawFlags = gDrawTestConfig[ "default" ];
}

static void Draw_Group( game_t& game,
                 const view_params_t& vp,
                 map_tile_list_t& billboards,
                 map_tile_list_t& walls,
                 map_tile_list_t& freeSpace );

static void Draw_Hud( const game_t& game );

static void Draw_Axes( const game_t& game, const view_params_t& vp );

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

    billTexture.mipmap = true;
    billTexture.LoadFromFile( "asset/mooninite.png" );
    billTexture.Load2D();

	groundPlane.normal = glm::vec3( 0.0f, 1.0f, 0.0f );
	groundPlane.d = 0.0f;

    gen.reset( new tile_generator_t( collision ) );

    std::sort( gen->freeSpace.begin(), gen->freeSpace.end(), []( const map_tile_t* a, const map_tile_t* b ) -> bool
	{
		glm::vec2 va( a->x, a->z ), vb( b->x, b->z );

		return glm::length( va ) < glm::length( vb );
	});

    const map_tile_t* tile = gen->freeSpace[ gen->freeSpace.size() / 2 ];

    auto LMakeBody = [ this, &tile ]( input_client_t& dest, input_client_t::mode_t mode, const glm::vec3& pos, float mass )
    {
        body_t* body = new body_t( body_t::RESET_VELOCITY_BIT | body_t::RESET_FORCE_ACCUM_BIT );
        body->SetPosition( pos );
        body->SetMass( mass );

        dest.mode = mode;
        dest.body.reset( body );
        dest.Sync();

        world.bodies.push_back( dest.body );
    };

    LMakeBody( player, input_client_t::MODE_PLAY, glm::vec3( tile->x, 0.0f, tile->z ), 80.0f );
    LMakeBody( spec, input_client_t::MODE_SPEC, glm::vec3( 0.0f, 10.0f, 0.0f ), 5.0f );

	camera = &player;
    drawBounds = spec.QueryBounds( ENTITY_BOUNDS_MOVE_COLLIDE )->ToBox();

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
    collision = collision_provider_t();

    gen.reset( new tile_generator_t( collision ) );
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

	if ( drawAll )
	{
		Draw_Group( *this, vp, gen->billboards, gen->walls, gen->freeSpace );
	}
	else
	{
		Draw_Group( *this, vp, billboards, walls, freeSpace );
	}
}

void game_t::FireGun( void )
{
    if ( camera->body )
    {
        bullet.reset( new entity_t( entity_t::BODY_DEPENDENT, new body_t ) );

        bullet->body->SetOrientation( glm::mat3( 0.1f ) * camera->body->GetOrientation() );
        bullet->body->ApplyVelocity( glm::vec3( 0.0f, 0.0f, -10.0f ) ); // Compensate for the applied scale of the bounds
        bullet->body->SetPosition( camera->GetViewParams().origin );

        bullet->AddBounds( ENTITY_BOUNDS_ALL, new bounding_box_t() );
    }
}

static void Draw_Bounds( const game_t& game, const bounding_box_t& bounds, const glm::vec3& color, float alpha = 1.0f )
{
    const shader_program_t& singleColor = game.pipeline->programs.at( "single_color" );
    const draw_buffer_t& coloredCube = game.pipeline->drawBuffers.at( "colored_cube" );
    const view_params_t& vp = game.camera->GetViewParams();

    singleColor.LoadVec4( "color", glm::vec4( color, alpha ) );
    singleColor.LoadMat4( "modelToView",  vp.transform * bounds.GetTransform() );
    coloredCube.Render( singleColor );
}

static void Draw_Hud( const game_t& game )
{
    // Clear depth buffer so the reticule renders over anything else it tests
    // against by default
    GL_CHECK( glClear( GL_DEPTH_BUFFER_BIT ) );

    const shader_program_t& ssSingleColor = game.pipeline->programs.at( "single_color_ss" );

    ssSingleColor.Bind();
    ssSingleColor.LoadVec4( "color", glm::vec4( 1.0f ) );

    imm_draw_t drawer( ssSingleColor );

    // Draw reticule
    drawer.Begin( GL_POINTS );
    drawer.Vertex( glm::vec3( 0.0f ) );
    drawer.End();

    ssSingleColor.Release();
}

static void Draw_Axes( const game_t& game, const view_params_t& vp )
{
    const shader_program_t& singleColor = game.pipeline->programs.at( "single_color" );

    singleColor.Bind();
    singleColor.LoadMat4( "modelToView", vp.transform * glm::translate( glm::mat4( 1.0f ), glm::vec3( 0.0f, 1.0f, 0.0f ) ) );
    singleColor.LoadVec4( "color", glm::vec4( 0.5f, 0.5f, 0.5f, 1.0f ) );

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


namespace {

uint32_t regionIter = 0;
float frameCount = 0.0f;

// For coloring arbitrary amounts of tiles
glm::mat4 quadTransform(
    []( void ) -> glm::mat4
    {
        // Rotate 90 degrees in the x-axis so the quad is on the ground, and then set the y-axis to -1
        // so that it's on the same level as the bottom of the bounding boxes ( i.e., walls )
        glm::mat4 t( glm::rotate( glm::mat4( 1.0f ), glm::half_pi< float >(), glm::vec3( 1.0f, 0.0f, 0.0f ) ) );
        t[ 3 ].y = -1.0f;
        return t;
    }()
);

void Apply_Force( game_t& game, const glm::vec3& normal, const body_t& body );

INLINE bool Tile_TestCollision(
    game_t& game,
    const entity_t* entity,
    const glm::vec3& entityPos,
    entity_bounds_use_flags_t entFlags,
    const map_tile_t* tile,
    entity_bounds_use_flags_t tileFlags )
{


    //const bounding_box_t& box = ;

    glm::mat4 t = M_TransformFromTile( *tile );

    if ( glm::distance( entityPos, glm::vec3( t[ 3 ] ) ) < 2.0f )
    {
        collision_entity_t ce( game.collision,
                               entity,
                               ( const entity_t* )tile,
                               entFlags,
                               tileFlags );

        if ( game.collision.EvalCollision( ce ) )
        {
            Draw_Bounds( game, *ENTITY_PTR_GET_BOX( tile, ENTITY_BOUNDS_AREA_EVAL ), glm::vec3( 0.0f, 1.0f, 0.0f ) );
            Apply_Force( game, ce.normal, *( tile->body ) );

            return true;
        }
    }

    return false;
}

void Draw_Quad( const game_t& game, const glm::mat4& transform, const glm::vec3& color, float alpha = 1.0f )
{
    const shader_program_t& singleColor = game.pipeline->programs.at( "single_color" );
    const draw_buffer_t& billboardBuffer = game.pipeline->drawBuffers.at( "billboard" );
    const view_params_t& vp = game.camera->GetViewParams();

    singleColor.Bind();
    singleColor.LoadVec4( "color", glm::vec4( color, alpha ) );
    singleColor.LoadMat4( "modelToView", vp.transform * transform * quadTransform );
    billboardBuffer.Render( singleColor );
    singleColor.Release();
}

INLINE void Draw_Tiles( const game_t& game,
                        const std::vector< const map_tile_t* >& tiles,
                        const glm::vec3& color, float alpha = 1.0f )
{
    for ( const map_tile_t* tile: tiles )
    {
        const bounding_box_t& box = *ENTITY_PTR_GET_BOX( tile, ENTITY_BOUNDS_AREA_EVAL );

        Draw_Quad( game, box.transform, color, alpha );
    }
}

INLINE void Draw_Tiles( const game_t& game, const std::vector< const map_tile_t* >& tiles, const glm::vec4& color )
{
    Draw_Tiles( game, tiles, glm::vec3( color ), color.a );
}

INLINE void Draw_AdjacentTiles( const game_t& game, const adjacent_region_list_t& adjRegions )
{
    for ( const adjacent_region_t& br: adjRegions )
    {
        auto adj = br.region.lock();

        if ( !adj )
        {
            continue;
        }

        Draw_Tiles( game, adj->tiles, glm::vec3( adj->color ), 1.0f );
    }
}

INLINE void Draw_BoundsTiles( const game_t& game, const shared_tile_region_t& region )
{
    load_blend_t blend( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    glm::vec3 color( 1.0f, 0.0f, 0.0f );

    for ( const adjacent_wall_t& br: region->wallTiles )
    {
        const bounding_box_t& box = *ENTITY_PTR_GET_BOX( br.source, ENTITY_BOUNDS_AREA_EVAL );

        Draw_Quad( game, box.transform, glm::vec3( 0.0f, 1.0f, 0.0f ), 1.0f );
        Draw_Tiles( game, br.walls, color, 1.0f );
    }

    assert( region->origin );

    const bounding_box_t& box = *ENTITY_PTR_GET_BOX( region->origin, ENTITY_BOUNDS_AREA_EVAL );

    Draw_Quad( game, box.transform, glm::vec3( 1.0f ), 1.0f );

    region->boundsVolume->root->Draw( *( game.pipeline ), game.camera->GetViewParams() );
}

INLINE bool InAdjacentRegionList( ref_tile_region_t source, const adjacent_region_list_t& adjRegions )
{
    for ( const adjacent_region_t& r: adjRegions )
    {
        if ( source == r.region )
        {
            return true;
        }
    }

    return false;
}

void Draw_Regions( game_t& game, bool drawBoundsTiles, bool drawAdjacent = false )
{
    for ( uint32_t i = 0; i < game.gen->regions.size(); ++i )
    {
        ref_tile_region_t weakRegion = game.gen->regions[ i ];
        auto region = weakRegion.lock();
        if ( !region )
        {
            continue;
        }

        bool canDraw = game.gen->regions[ regionIter ] != region
                && !InAdjacentRegionList( weakRegion, game.gen->regions[ regionIter ]->adjacent );

        // If drawAdjacent is turned on, then we cannot draw
        // the regions as normal if i == regionIter: for some reason,
        // despite being rendered previously, this pass will overwrite
        // following draw pass in the color buffer, which produces inaccurate results.
        if ( canDraw || ( drawBoundsTiles && !drawAdjacent ) )
        {
            Draw_Tiles( game, region->tiles, region->color );
        }

        if ( i == regionIter && ( drawBoundsTiles || drawAdjacent ) )
        {
            // Note: if drawAdjacent is turned on,
            // it's pretty hard seeing which tile is current
            // without a full alpha channel.

            if ( drawAdjacent )
            {
                Draw_AdjacentTiles( game, region->adjacent );
            }

            // Draw these last since the adjacent regions take up the most space
            if ( drawBoundsTiles )
            {
                Draw_BoundsTiles( game, region );
            }
        }
    }
}

void Apply_Force( game_t& game, const glm::vec3& normal, const body_t& body )
{
    if ( game.camera->body )
    {
       game.camera->body->ApplyForce( P_GenericCollideNormal( normal, body, *( game.camera->body ) ) );
    }
}

INLINE void Billboard_DrawBounds( const game_t& game, const view_params_t& vp, const shader_program_t& singleColor, const bounding_box_t& billboardBounds )
{
    singleColor.Bind();
    Draw_Bounds( game, billboardBounds, glm::vec3( 0.5f ), 0.5f );

    singleColor.LoadVec4( "color", glm::vec4( 1.0f, 0.0f, 0.0f, 1.0f ) );
    singleColor.LoadMat4( "modelToView", vp.transform * billboardBounds.transform );

    imm_draw_t drawer( singleColor );

    drawer.Begin( GL_LINES );
    drawer.Vertex( glm::vec3( 0.0f ) );
    drawer.Vertex( glm::vec3( 0.0f, 0.0f, 3.0f ) );
    drawer.End();
}

INLINE void Billboard_LoadParams( map_tile_t& tile, const view_params_t& vp, const shader_program_t& billboard )
{
    const bounding_box_t& bounds = *( tile.QueryBounds( ENTITY_BOUNDS_AREA_EVAL )->ToBox() );

    glm::vec3 boundsOrigin( bounds[ 3 ] );

    billboard.LoadVec3( "origin", boundsOrigin );

    glm::vec3 dirToCam( vp.origin - boundsOrigin );
    dirToCam.y = 0.0f;
    dirToCam = glm::normalize( dirToCam );

    glm::mat3 orient(
        G_OrientByDirection(
                dirToCam,
                glm::vec3( 0.0f, 0.0f, 1.0f ),
                glm::vec3( -1.0f, 0.0f, 0.0f )
        )
    );

    // This load ensures that the billboard is always facing the viewer
    billboard.LoadMat3( "viewOrient", orient );

    // Set orientation so collisions are properly computed regardless of direction
    tile.body->SetOrientation( orient );
    tile.Sync();
}

INLINE void Billboard_TestBulletCollision( game_t& game, map_tile_t* tile )
{
    if ( game.bullet )
    {
        if( Tile_TestCollision( game,
                            game.bullet.get(),
                            game.bullet->body->GetPosition(),
                            ENTITY_BOUNDS_AIR_COLLIDE,
                            tile,
                            ENTITY_BOUNDS_AIR_COLLIDE ) )
        {
            game.bullet.release();
        }
    }
}

void Process_Billboards( game_t& game, const view_params_t& vp, map_tile_list_t& billboards )
{
    const shader_program_t& singleColor = game.pipeline->programs.at( "single_color" );
    const shader_program_t& billboard = game.pipeline->programs.at( "billboard" );
    const draw_buffer_t& billboardBuffer = game.pipeline->drawBuffers.at( "billboard" );

    load_blend_t blend( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    // Draw all billboards
    billboard.Bind();
    billboard.LoadMat4( "modelToView", vp.transform );
    billboard.LoadMat4( "viewToClip", vp.clipTransform );

    for ( map_tile_t* tile: billboards )
    {
        Billboard_LoadParams( *tile, vp, billboard  );

        bounding_box_t* tileBounds = ENTITY_PTR_GET_BOX( tile, ENTITY_BOUNDS_MOVE_COLLIDE );

        if ( glm::distance( tile->body->GetPosition(), vp.origin ) <= DISTANCE_THRESHOLD )
        {
            // Check for an intersection...
            glm::vec3 normal;

            bounding_box_t* box = ENTITY_PTR_GET_BOX( game.camera, ENTITY_BOUNDS_MOVE_COLLIDE );

            if ( box->IntersectsBounds( normal, *tileBounds ) )
            {
                Apply_Force( game, normal, *( tile->body ) );
            }
        }

        singleColor.Bind();
        Billboard_TestBulletCollision( game, tile );
        billboard.Bind();

        billboard.LoadVec4( "color", tile->color );

        game.billTexture.Bind( 0, "image", billboard );
        billboardBuffer.Render( billboard );
        game.billTexture.Release( 0 );

        if ( gDrawFlags & DRAW_BILLBOARD_BOUNDS )
        {
            Billboard_DrawBounds( game, vp, singleColor, *tileBounds );
        }

        billboard.Bind();
    }
    billboard.Release();
}

} // end namespace



static void Draw_Group( game_t& game,
                 const view_params_t& vp,
                 map_tile_list_t& billboards,
                 map_tile_list_t& walls,
                 map_tile_list_t& freeSpace )
{
    UNUSEDPARAM( freeSpace );

    const shader_program_t& singleColor = game.pipeline->programs.at( "single_color" );

    //const draw_buffer_t& billboardBuffer = game.pipeline->drawBuffers.at( "billboard" );

    // immDrawer can be used in some arbitrary code block that is aware of the renderer to draw something.
    // It's useful for debugging...
	imm_draw_t drawer( singleColor );
	immDrawer = &drawer;

    // Load a grey color so it looks somewhat fancy
	singleColor.Bind();
    //singleColor.LoadVec4( "color", glm::vec4( 0.5f, 0.5f, 0.5f, 1.0f ) );

    if ( gDrawFlags & DRAW_WALLS )
    {
        for ( const map_tile_t* tile: walls )
        {
            Draw_Bounds( game, *ENTITY_PTR_GET_BOX( tile, ENTITY_BOUNDS_AREA_EVAL ), glm::vec3( 0.5f ) );

            Tile_TestCollision( game,
             ( const entity_t* )game.camera,
                                vp.origin,
                                ENTITY_BOUNDS_MOVE_COLLIDE,
                                tile,
                                ENTITY_BOUNDS_MOVE_COLLIDE  );
        }
    }

    if ( game.bullet )
    {
        Draw_Bounds( game, *( ENTITY_PTR_GET_BOX( game.bullet, ENTITY_BOUNDS_ALL) ), glm::vec3( 1.0f, 0.0f, 0.0f ) );
    }

    if ( gDrawFlags & DRAW_BILLBOARDS )
    {
        Process_Billboards( game, vp, billboards );
    }

    singleColor.Bind();
    singleColor.LoadMat4( "modelToView", vp.transform );
    singleColor.LoadVec4( "color", glm::vec4( 0.0f, 1.0f, 0.0f, 1.0f ) );

    if ( gDrawFlags & DRAW_HALFSPACES )
    {
        for ( const map_tile_t* wall: walls )
        {
            if ( wall->halfSpaceIndex < 0 )
            {
                continue;
            }

            const collision_face_table_t& table =
                    game.collision.halfSpaceTable[ wall->halfSpaceIndex ];

            for ( int32_t i: table )
            {
                if ( i < 0 )
                {
                    continue;
                }

                game.collision.halfSpaces[ i ].Draw( drawer );
            }
        }
    }

    // Draw the bounds of the camera not currently being used
    Draw_Bounds( game, *( game.drawBounds ), glm::vec3( 1.0f, 0.0f, 1.0f ) );

    singleColor.Bind();

    bool drawAdj = !!( gDrawFlags & DRAW_REGIONS_ADJACENT );
    bool drawBounds = !!( gDrawFlags & DRAW_REGIONS_BOUNDS );

    if  ( drawAdj || drawBounds )
    {
        Draw_Regions( game, drawBounds, drawAdj );
    }

    frameCount += 1.0f;

    if ( frameCount >= ( 5.0f / game.world.time ))
    {
        regionIter++;
        frameCount = 0.0f;
    }

    if ( regionIter == game.gen->regions.size() )
    {
        regionIter = 0;
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
                                game.drawBounds = ENTITY_GET_BOX( game.player, ENTITY_BOUNDS_MOVE_COLLIDE );
								game.camera = &game.spec;
							}
							else
							{
                                game.drawBounds = ENTITY_GET_BOX( game.spec, ENTITY_BOUNDS_MOVE_COLLIDE );
								game.camera = &game.player;
							}
							break;
                        case SDLK_b:
                            gDrawFlags ^= DRAW_BILLBOARDS;
                            break;

                        case SDLK_h:
                            gDrawFlags ^= DRAW_HALFSPACES;
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

