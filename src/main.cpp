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

static void Draw_Group( application& game,
                 const view_params_t& vp,
                 map_tile_list_t& billboards,
                 map_tile_list_t& walls,
                 map_tile_list_t& freeSpace );

static void Draw_Hud( const application& game );

static void Draw_Axes( const application& game, const view_params_t& vp );

application::application( uint32_t width_ , uint32_t height_ )
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

    gen.reset( new map_tile_generator( collision ) );

    std::sort( gen->mFreeSpace.begin(), gen->mFreeSpace.end(), []( const map_tile* a, const map_tile* b ) -> bool
	{
        glm::vec2 va( a->mX, a->mZ ), vb( b->mX, b->mZ );

		return glm::length( va ) < glm::length( vb );
	});

    const map_tile* tile = gen->mFreeSpace[ gen->mFreeSpace.size() / 2 ];

    auto LMakeBody = [ this, &tile ]( input_client& dest, input_client::client_mode mode, const glm::vec3& pos, float mass )
    {
        rigid_body* body = new rigid_body( rigid_body::RESET_VELOCITY_BIT | rigid_body::RESET_FORCE_ACCUM_BIT );
        body->position( pos );
        body->mass( mass );

        dest.mode = mode;
        dest.mBody.reset( body );
        dest.sync();

        world.mBodies.push_back( dest.mBody );
    };

    LMakeBody( player, input_client::MODE_PLAY, glm::vec3( tile->mX, 0.0f, tile->mZ ), 80.0f );
    LMakeBody( spec, input_client::MODE_SPEC, glm::vec3( 0.0f, 10.0f, 0.0f ), 5.0f );

	camera = &player;
    drawBounds = spec.query_bounds( ENTITY_BOUNDS_MOVE_COLLIDE )->to_box();

	running = true;
}

application::~application( void )
{
	SDL_Quit();
}

application& application::get_instance( void )
{
    static application game( 1366, 768 );
	return game;
}

void application::reset_map( void )
{
    collision = collision_provider();

    gen.reset( new map_tile_generator( collision ) );
}

void application::toggle_culling( void )
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

void application::tick( void )
{
	frameTime = startTime - lastTime;
}

void application::draw( void )
{
    const view_params_t& vp = camera->GetViewParams();

	GL_CHECK( glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ) );

	if ( drawAll )
	{
        Draw_Group( *this, vp, gen->mBillboards, gen->mWalls, gen->mFreeSpace );
	}
	else
	{
		Draw_Group( *this, vp, billboards, walls, freeSpace );
	}
}

void application::fire_gun( void )
{
    if ( camera->mBody )
    {
        bullet.reset( new entity( entity::BODY_DEPENDENT, new rigid_body ) );

        bullet->mBody->orientation( glm::mat3( 0.1f ) * camera->mBody->orientation() );
        bullet->mBody->apply_velocity( glm::vec3( 0.0f, 0.0f, -10.0f ) ); // Compensate for the applied scale of the bounds
        bullet->mBody->position( camera->GetViewParams().origin );

        bullet->add_bounds( ENTITY_BOUNDS_ALL, new obb() );
    }
}

static void Draw_Bounds( const application& game, const obb& bounds, const glm::vec3& color, float alpha = 1.0f )
{
    const shader_program_t& singleColor = game.pipeline->programs.at( "single_color" );
    const draw_buffer_t& coloredCube = game.pipeline->drawBuffers.at( "colored_cube" );
    const view_params_t& vp = game.camera->GetViewParams();

    singleColor.LoadVec4( "color", glm::vec4( color, alpha ) );
    singleColor.LoadMat4( "modelToView",  vp.transform * bounds.axes() );
    coloredCube.Render( singleColor );
}

static void Draw_Hud( const application& game )
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

static void Draw_Axes( const application& game, const view_params_t& vp )
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

void Apply_Force( application& game, const glm::vec3& normal, const rigid_body& body );

INLINE bool Tile_TestCollision(
    application& game,
    const entity* e,
    const glm::vec3& entityPos,
    entity_bounds_use_flags entFlags,
    const map_tile* tile,
    entity_bounds_use_flags tileFlags )
{


    //const bounding_box_t& box = ;

    glm::mat4 t = get_tile_transform( *tile );

    if ( glm::distance( entityPos, glm::vec3( t[ 3 ] ) ) < 2.0f )
    {
        collision_entity ce( game.collision,
                               e,
                               ( const entity* )tile,
                               entFlags,
                               tileFlags );

        if ( game.collision.EvalCollision( ce ) )
        {
            Draw_Bounds( game, *ENTITY_PTR_GET_BOX( tile, ENTITY_BOUNDS_AREA_EVAL ), glm::vec3( 0.0f, 1.0f, 0.0f ) );
            Apply_Force( game, ce.normal, *( tile->mBody ) );

            return true;
        }
    }

    return false;
}

void Draw_Quad( const application& game, const glm::mat4& transform, const glm::vec3& color, float alpha = 1.0f )
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

INLINE void Draw_Tiles( const application& game,
                        const std::vector< const map_tile* >& tiles,
                        const glm::vec3& color, float alpha = 1.0f )
{
    for ( const map_tile* tile: tiles )
    {
        const obb& box = *ENTITY_PTR_GET_BOX( tile, ENTITY_BOUNDS_AREA_EVAL );

        Draw_Quad( game, box.mAxes, color, alpha );
    }
}

INLINE void Draw_Tiles( const application& game, const std::vector< const map_tile* >& tiles, const glm::vec4& color )
{
    Draw_Tiles( game, tiles, glm::vec3( color ), color.a );
}

INLINE void Draw_AdjacentTiles( const application& game, const adjacent_region_list_t& adjRegions )
{
    for ( const adjacent_region& br: adjRegions )
    {
        auto adj = br.mRegion.lock();

        if ( !adj )
        {
            continue;
        }

        Draw_Tiles( game, adj->mTiles, glm::vec3( adj->mColor ), 1.0f );
    }
}

INLINE void Draw_BoundsTiles( const application& game, const shared_tile_region_t& region )
{
    load_blend_t blend( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    glm::vec3 color( 1.0f, 0.0f, 0.0f );

    for ( const adjacent_wall& br: region->mWalls )
    {
        const obb& box = *ENTITY_PTR_GET_BOX( br.mSource, ENTITY_BOUNDS_AREA_EVAL );

        Draw_Quad( game, box.mAxes, glm::vec3( 0.0f, 1.0f, 0.0f ), 1.0f );
        Draw_Tiles( game, br.mWalls, color, 1.0f );
    }

    assert( region->mOrigin );

    const obb& box = *ENTITY_PTR_GET_BOX( region->mOrigin, ENTITY_BOUNDS_AREA_EVAL );

    Draw_Quad( game, box.mAxes, glm::vec3( 1.0f ), 1.0f );

    region->mBoundsVolume->root->Draw( *( game.pipeline ), game.camera->GetViewParams() );
}

INLINE bool InAdjacentRegionList( ref_tile_region_t source, const adjacent_region_list_t& adjRegions )
{
    for ( const adjacent_region& r: adjRegions )
    {
        if ( source == r.mRegion )
        {
            return true;
        }
    }

    return false;
}

void Draw_Regions( application& game, bool drawBoundsTiles, bool drawAdjacent = false )
{
    for ( uint32_t i = 0; i < game.gen->mRegions.size(); ++i )
    {
        ref_tile_region_t weakRegion = game.gen->mRegions[ i ];
        auto region = weakRegion.lock();
        if ( !region )
        {
            continue;
        }

        bool canDraw = game.gen->mRegions[ regionIter ] != region
                && !InAdjacentRegionList( weakRegion, game.gen->mRegions[ regionIter ]->mAdjacent );

        // If drawAdjacent is turned on, then we cannot draw
        // the regions as normal if i == regionIter: for some reason,
        // despite being rendered previously, this pass will overwrite
        // following draw pass in the color buffer, which produces inaccurate results.
        if ( canDraw || ( drawBoundsTiles && !drawAdjacent ) )
        {
            Draw_Tiles( game, region->mTiles, region->mColor );
        }

        if ( i == regionIter && ( drawBoundsTiles || drawAdjacent ) )
        {
            // Note: if drawAdjacent is turned on,
            // it's pretty hard seeing which tile is current
            // without a full alpha channel.

            if ( drawAdjacent )
            {
                Draw_AdjacentTiles( game, region->mAdjacent );
            }

            // Draw these last since the adjacent regions take up the most space
            if ( drawBoundsTiles )
            {
                Draw_BoundsTiles( game, region );
            }
        }
    }
}

void Apply_Force( application& game, const glm::vec3& normal, const rigid_body& body )
{
    if ( game.camera->mBody )
    {
       game.camera->mBody->apply_force( get_collision_normal( normal, body, *( game.camera->mBody ) ) );
    }
}

INLINE void Billboard_DrawBounds( const application& game, const view_params_t& vp, const shader_program_t& singleColor, const obb& billboardBounds )
{
    singleColor.Bind();
    Draw_Bounds( game, billboardBounds, glm::vec3( 0.5f ), 0.5f );

    singleColor.LoadVec4( "color", glm::vec4( 1.0f, 0.0f, 0.0f, 1.0f ) );
    singleColor.LoadMat4( "modelToView", vp.transform * billboardBounds.mAxes );

    imm_draw_t drawer( singleColor );

    drawer.Begin( GL_LINES );
    drawer.Vertex( glm::vec3( 0.0f ) );
    drawer.Vertex( glm::vec3( 0.0f, 0.0f, 3.0f ) );
    drawer.End();
}

INLINE void Billboard_LoadParams( map_tile& tile, const view_params_t& vp, const shader_program_t& billboard )
{
    const obb& bounds = *( tile.query_bounds( ENTITY_BOUNDS_AREA_EVAL )->to_box() );

    glm::vec3 boundsOrigin( bounds[ 3 ] );

    billboard.LoadVec3( "origin", boundsOrigin );

    glm::vec3 dirToCam( vp.origin - boundsOrigin );
    dirToCam.y = 0.0f;
    dirToCam = glm::normalize( dirToCam );

    glm::mat3 orient(
        orient_by_direction(
                dirToCam,
                glm::vec3( 0.0f, 0.0f, 1.0f ),
                glm::vec3( -1.0f, 0.0f, 0.0f )
        )
    );

    // This load ensures that the billboard is always facing the viewer
    billboard.LoadMat3( "viewOrient", orient );

    // Set orientation so collisions are properly computed regardless of direction
    tile.mBody->orientation( orient );
    tile.sync();
}

INLINE void Billboard_TestBulletCollision( application& game, map_tile* tile )
{
    if ( game.bullet )
    {
        if( Tile_TestCollision( game,
                            game.bullet.get(),
                            game.bullet->mBody->position(),
                            ENTITY_BOUNDS_AIR_COLLIDE,
                            tile,
                            ENTITY_BOUNDS_AIR_COLLIDE ) )
        {
            game.bullet.release();
        }
    }
}

void Process_Billboards( application& game, const view_params_t& vp, map_tile_list_t& billboards )
{
    const shader_program_t& singleColor = game.pipeline->programs.at( "single_color" );
    const shader_program_t& billboard = game.pipeline->programs.at( "billboard" );
    const draw_buffer_t& billboardBuffer = game.pipeline->drawBuffers.at( "billboard" );

    load_blend_t blend( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    // Draw all billboards
    billboard.Bind();
    billboard.LoadMat4( "modelToView", vp.transform );
    billboard.LoadMat4( "viewToClip", vp.clipTransform );

    for ( map_tile* tile: billboards )
    {
        Billboard_LoadParams( *tile, vp, billboard  );

        obb* tileBounds = ENTITY_PTR_GET_BOX( tile, ENTITY_BOUNDS_MOVE_COLLIDE );

        if ( glm::distance( tile->mBody->position(), vp.origin ) <= DISTANCE_THRESHOLD )
        {
            // Check for an intersection...
            glm::vec3 normal;

            obb* box = ENTITY_PTR_GET_BOX( game.camera, ENTITY_BOUNDS_MOVE_COLLIDE );

            if ( box->intersects( normal, *tileBounds ) )
            {
                Apply_Force( game, normal, *( tile->mBody ) );
            }
        }

        singleColor.Bind();
        Billboard_TestBulletCollision( game, tile );
        billboard.Bind();

        billboard.LoadVec4( "color", tile->mColor );

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



static void Draw_Group( application& game,
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
        for ( const map_tile* tile: walls )
        {
            Draw_Bounds( game, *ENTITY_PTR_GET_BOX( tile, ENTITY_BOUNDS_AREA_EVAL ), glm::vec3( 0.5f ) );

            Tile_TestCollision( game,
             ( const entity* )game.camera,
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
        for ( const map_tile* wall: walls )
        {
            if ( wall->mHalfSpaceIndex < 0 )
            {
                continue;
            }

            const collision_face_table_t& table =
                    game.collision.halfSpaceTable[ wall->mHalfSpaceIndex ];

            for ( int32_t i: table )
            {
                if ( i < 0 )
                {
                    continue;
                }

                game.collision.halfSpaces[ i ].draw( drawer );
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

    if ( frameCount >= ( 5.0f / game.world.mTime ))
    {
        regionIter++;
        frameCount = 0.0f;
    }

    if ( regionIter == game.gen->mRegions.size() )
    {
        regionIter = 0;
    }

	singleColor.Release();
}

void Game_Frame( void )
{
    application& game = application::get_instance();

	game.startTime = get_time();

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
        game.gen->find_entities( game.billboards, game.walls, game.freeSpace, game.frustum, vp );
	}

    game.world.update( game );
    game.draw();

    // clear bodies which are added in world.Update call,
    // since we only want to integrate bodies which are in view
    game.world.mBodies.clear();

    game.world.mTime = get_time() - game.startTime;

    printf( "FPS: %f\r", 1.0f / game.world.mTime );
}

// temporary hack to get around the fact that querying for an game
// instance if an error is hgameening causes problems; we use this flag the exit.
static bool* runningPtr = nullptr;

uint32_t Game_Exec( void )
{   	
    application& game = application::get_instance();
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
                            game.reset_map();
							break;
						case SDLK_v:
                            game.toggle_culling();
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
                            game.camera->EvalKeyPress( ( input_key ) e.key.keysym.sym );
                            break;


                    }
                    break;
                case SDL_KEYUP:
                    game.camera->EvalKeyRelease( ( input_key ) e.key.keysym.sym );
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
                        game.fire_gun();
                    }
                    break;
            }
        }
    }
#endif

    return 0;
}

void flag_exit( void )
{
	*runningPtr = false;
}

float get_time( void )
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

