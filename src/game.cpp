#include "game.h"

#include <iostream>

#include "renderer.h"
#include "debug.h"

#ifdef OP_UNIX
	#include <unistd.h>
#endif

#include <stdint.h>
#include <array>
#include <cstdlib>
#include <unordered_map>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtx/projection.hpp>
#include <glm/gtc/quaternion.hpp>

void exec_frame( void );

#include "eminput.h"

unsigned int gDebugFlag = 0;

namespace {
    enum
    {
        DRAW_BILLBOARDS = 1 << 0,
        DRAW_BILLBOARD_BOUNDS = 1 << 1,
        DRAW_HALFSPACES = 1 << 2,
        DRAW_REGIONS_ADJACENT = 1 << 3,
        DRAW_REGIONS_BOUNDS = 1 << 4,
        DRAW_WALLS = 1 << 5,
        DRAW_QUAD_REGIONS = 1 << 6,
		DRAW_CANCEL_REGIONS = 1 << 7,
		COLLIDE_BILLBOARDS = 1 << 8,
		COLLIDE_WALLS = 1 << 9
    };

    const float DISTANCE_THRESHOLD = 2.0f;

	std::unordered_map< std::string, uint32_t > gTestConfig =
    {
        { "adjacency_test", DRAW_REGIONS_ADJACENT | DRAW_WALLS },
        { "default", DRAW_BILLBOARDS | DRAW_WALLS | DRAW_HALFSPACES | DRAW_BILLBOARD_BOUNDS },
		{ "collision_test", DRAW_BILLBOARDS | DRAW_HALFSPACES | COLLIDE_BILLBOARDS | COLLIDE_WALLS },
        { "bounds_tiles_test", DRAW_REGIONS_BOUNDS | DRAW_CANCEL_REGIONS }
    };

	uint32_t gTestFlags = gTestConfig[ "default" ];

}

game::game( uint32_t width, uint32_t height )
    : application( width, height )
{
    billTexture.mip_map( true );
    billTexture.open_file( "asset/mooninite.png" );
    billTexture.load_2d();

    gen.reset( new map_tile_generator( collision ) );

    std::sort( gen->mFreeSpace.begin(), gen->mFreeSpace.end(), []( const map_tile* a, const map_tile* b ) -> bool
    {
        glm::vec2 va( a->mX, a->mZ ), vb( b->mX, b->mZ );

        return glm::length( va ) < glm::length( vb );
    });

    const map_tile* tile = gen->mFreeSpace[ gen->mFreeSpace.size() / 2 ];

    make_body( player, input_client::MODE_PLAY, glm::vec3( tile->mX, 0.0f, tile->mZ ), 80.0f );
    make_body( spec, input_client::MODE_SPEC, glm::vec3( 0.0f, 10.0f, 0.0f ), 5.0f );

    fill_orient_map();
}

void game::fill_orient_map( void )
{
    mBillboardsOriented.clear();

    for ( const map_tile* b: gen->mBillboards )
    {
        billboard_oriented( *b, true );
    }
}

void game::reset_map( void )
{
    collision = collision_provider();

    gen.reset( new map_tile_generator( collision ) );
    fill_orient_map();
}

void game::fire_gun( void )
{
    if ( camera->mBody )
    {
        debug_set_flag( false );
        bullet.reset( new entity( entity::BODY_DEPENDENT, new rigid_body() ) );

        bullet->mSize = 0.1f;
        bullet->sync_options( ENTITY_SYNC_APPLY_SCALE );

        bullet->mBody->orientation( camera->mBody->orientation_mat3() );
        bullet->mBody->apply_velocity( camera->view_params().mForward );
        bullet->mBody->position( camera->view_params().mOrigin );

        bullet->add_bounds( ENTITY_BOUNDS_ALL, new obb() );
    }
}

static void draw_group( application& game,
                 const view_data& vp,
                 map_tile_list_t& billboards,
                 map_tile_list_t& walls,
                 map_tile_list_t& freeSpace );

static void draw_hud( const application& game );

static void draw_axes( const application& game, const view_data& vp );



static void draw_bounds( const application& game, const obb& bounds, const glm::vec3& color, float alpha = 1.0f )
{
    const shader_program& singleColor = game.pipeline->programs().at( "single_color" );
    const draw_buffer& coloredCube = game.pipeline->draw_buffers().at( "colored_cube" );
    const view_data& vp = game.camera->view_params();

    singleColor.load_vec4( "color", glm::vec4( color, alpha ) );
    singleColor.load_mat4( "modelToView",  vp.mTransform * bounds.axes() );
    coloredCube.render( singleColor );
}

static void draw_hud( const application& game )
{
    // Clear depth buffer so the reticule renders over anything else it tests
    // against by default
    GL_CHECK( glClear( GL_DEPTH_BUFFER_BIT ) );

    const shader_program& ssSingleColor = game.pipeline->programs().at( "single_color_ss" );

    ssSingleColor.bind();
    ssSingleColor.load_vec4( "color", glm::vec4( 1.0f ) );

    imm_draw drawer( ssSingleColor );

    // Draw reticule
    drawer.begin( GL_POINTS );
    drawer.vertex( glm::vec3( 0.0f ) );
    drawer.end();

    ssSingleColor.release();
}

static void draw_axes( const application& game, const view_data& vp )
{
    const shader_program& singleColor = game.pipeline->programs().at( "single_color" );

    singleColor.bind();
    singleColor.load_mat4( "modelToView", vp.mTransform * glm::translate( glm::mat4( 1.0f ), glm::vec3( 0.0f, 1.0f, 0.0f ) ) );
    singleColor.load_vec4( "color", glm::vec4( 0.5f, 0.5f, 0.5f, 1.0f ) );

    imm_draw drawer( singleColor );

    const float AXIS_SIZE = 100.0f;

    // Draw coordinate space axes
    drawer.begin( GL_LINES );
    drawer.vertex( glm::vec3( 0.0f ) );
    drawer.vertex( glm::vec3( 0.0f, 0.0f, AXIS_SIZE ) );
    drawer.vertex( glm::vec3( 0.0f ) );
    drawer.vertex( glm::vec3( 0.0f, AXIS_SIZE, 0.0f ) );
    drawer.vertex( glm::vec3( 0.0f ) );
    drawer.vertex( glm::vec3( AXIS_SIZE, 0.0f, 0.0f ) );
    drawer.end();

    singleColor.release();
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

void apply_force( application& game, const collision_entity& ce );

INLINE bool test_tile_collision(
    application& game,
	collision_entity::ptr_t e,
    const glm::vec3& entityPos,
    entity_bounds_use_flags entFlags,
    const map_tile* tile,
    entity_bounds_use_flags tileFlags )
{
    glm::mat4 t = get_tile_transform( *tile );

    if ( glm::distance( entityPos, glm::vec3( t[ 3 ] ) ) < 2.0f )
    {
        collision_entity ce( game.collision,
                               e,
							   ( collision_entity::ptr_t )tile,
                               entFlags,
                               tileFlags );

        if ( game.collision.EvalCollision( ce ) )
        {
            draw_bounds( game, *ENTITY_PTR_GET_BOX( tile, ENTITY_BOUNDS_AREA_EVAL ), glm::vec3( 0.0f, 1.0f, 0.0f ) );

            if ( tile->mType != map_tile::BILLBOARD )
            {
                apply_force( game, ce );
            }

            return true;
        }
    }

    return false;
}

void draw_quad( const application& game, const glm::mat4& transform, const glm::vec3& color, float alpha = 1.0f )
{
    const shader_program& singleColor = game.pipeline->programs().at( "single_color" );
    const draw_buffer& billboardBuffer = game.pipeline->draw_buffers().at( "billboard" );
    const view_data& vp = game.camera->view_params();

    singleColor.bind();
    singleColor.load_vec4( "color", glm::vec4( color, alpha ) );
    singleColor.load_mat4( "modelToView", vp.mTransform * transform * quadTransform );
    billboardBuffer.render( singleColor );
    singleColor.release();
}

INLINE void draw_tiles( const application& game,
                        const std::vector< const map_tile* >& tiles,
                        const glm::vec3& color, float alpha = 1.0f )
{
    for ( const map_tile* tile: tiles )
    {
        const obb& box = *ENTITY_PTR_GET_BOX( tile, ENTITY_BOUNDS_AREA_EVAL );

        draw_quad( game, box.axes(), color, alpha );
    }
}

INLINE void draw_tiles( const application& game, const std::vector< const map_tile* >& tiles, const glm::vec4& color )
{
    draw_tiles( game, tiles, glm::vec3( color ), color.a );
}

INLINE void draw_adjacent_tiles( const application& game, const adjacent_region_list_t& adjRegions )
{
    for ( const adjacent_region& br: adjRegions )
    {
        auto adj = br.mRegion.lock();

        if ( !adj )
        {
            continue;
        }

        draw_tiles( game, adj->mTiles, glm::vec3( adj->mColor ), 1.0f );
    }
}

INLINE void draw_bounds_tiles( const application& game, const shared_tile_region_t& region )
{
    /*
    set_blend_mode blend( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    glm::vec3 color( 1.0f, 0.0f, 0.0f );

    for ( const adjacent_wall& br: region->mWalls )
    {
        const obb& box = *ENTITY_PTR_GET_BOX( br.mSource, ENTITY_BOUNDS_AREA_EVAL );

        Draw_Quad( game, box.mAxes, glm::vec3( 0.0f, 1.0f, 0.0f ), 1.0f );
        Draw_Tiles( game, br.mWalls, color, 1.0f );
    }

    assert( region->mOrigin );
*/
    const obb& box = *ENTITY_PTR_GET_BOX( region->mOrigin, ENTITY_BOUNDS_AREA_EVAL );

    draw_quad( game, box.axes(), glm::vec3( 1.0f ), 1.0f );

    region->mBoundsVolume->mRoot->draw( *( game.pipeline ), game.camera->view_params() );
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

bool HasCancelRegions( void )
{
	return !!( gTestFlags & DRAW_CANCEL_REGIONS );
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
                && !InAdjacentRegionList( weakRegion, game.gen->mRegions[ regionIter ]->mAdjacent )
                && !HasCancelRegions();

        // If drawAdjacent is turned on, then we cannot draw
        // the regions as normal if i == regionIter: for some reason,
        // despite being rendered previously, this pass will overwrite
        // following draw pass in the color buffer, which produces inaccurate results.
        if ( canDraw || ( drawBoundsTiles && !drawAdjacent && !HasCancelRegions() ) )
        {
            draw_tiles( game, region->mTiles, region->mColor );
        }

        if ( i == regionIter && ( drawBoundsTiles || drawAdjacent ) )
        {
            // Note: if drawAdjacent is turned on,
            // it's pretty hard seeing which tile is current
            // without a full alpha channel.

            if ( drawAdjacent )
            {
                draw_adjacent_tiles( game, region->mAdjacent );
            }

            // Draw these last since the adjacent regions take up the most space
            if ( drawBoundsTiles )
            {
                draw_bounds_tiles( game, region );
            }
        }
    }
}

void apply_force( application& game, const collision_entity& ce )
{
	if ( ce.collider->mBody )
    {
		glm::vec3 offset( ce.normal * ce.interpenDepth );

		game.camera->mBody->apply_force(
					get_collision_normal( offset, *( ce.collidee->mBody ), *( ce.collider->mBody ) ) );
    }
}

INLINE void draw_billboard_bounds( const application& game, const view_data& vp, const shader_program& singleColor, const obb& billboardBounds )
{
    singleColor.bind();
    draw_bounds( game, billboardBounds, glm::vec3( 0.5f ), 0.5f );

    singleColor.load_vec4( "color", glm::vec4( 1.0f, 0.0f, 0.0f, 1.0f ) );
    singleColor.load_mat4( "modelToView", vp.mTransform * billboardBounds.axes() );

    imm_draw drawer( singleColor );

    drawer.begin( GL_LINES );
    drawer.vertex( glm::vec3( 0.0f ) );
    drawer.vertex( glm::vec3( 0.0f, 0.0f, 3.0f ) );
    drawer.end();
}

INLINE void load_billboard_params( map_tile& tile, const shader_program& billboard )
{
    const obb& bounds = *( tile.query_bounds( ENTITY_BOUNDS_AREA_EVAL )->to_box() );

    glm::vec3 boundsOrigin( bounds[ 3 ] );

    billboard.load_vec3( "origin", boundsOrigin );

    billboard.load_mat3( "viewOrient", tile.mBody->orientation_mat3() );
}

INLINE void draw_debug_ray( application& game,
							imm_draw& d,
							const obb& bounds,
							const shader_program& singleColor,
							const ray& debugRay )
{
	singleColor.load_mat4( "modelToView", game.camera->view_params().mTransform );

	singleColor.load_vec4( "color", glm::vec4( 1.0f ) );

	d.begin( GL_LINES );
	d.vertex( debugRay.p );
	d.vertex( debugRay.calc_position() );
	d.end();

	singleColor.load_vec4( "color", glm::vec4( 1.0f, 0.0f, 0.0f, 1.0f ) );

	d.begin( GL_POINTS );
	d.vertex( debugRay.p );
	d.end();

	singleColor.load_vec4( "color", glm::vec4( 0.0f, 0.0f, 1.0f, 1.0f ) );

	d.begin( GL_POINTS );
	d.vertex( debugRay.calc_position() );
	d.end();

    obb::maxmin_pair3D_t mm = bounds.maxmin( false );

	singleColor.load_vec4( "color", glm::vec4( 1.0f, 0.0f, 1.0f, 1.0f ) );

	d.begin( GL_POINTS );
	d.vertex( mm.max );
	d.end();

	singleColor.load_vec4( "color", glm::vec4( 0.0f, 1.0f, 1.0f, 1.0f ) );

	d.begin( GL_POINTS );
	d.vertex( mm.min );
	d.end();
}

INLINE void draw_debug_ray_list( application& game, const obb& bounds, const shader_program& singleColor )
{
	imm_draw d( singleColor );

	for ( auto i = debug_raylist_begin(); i != debug_raylist_end(); ++i )
	{
		draw_debug_ray( game, d, bounds, singleColor, *i );
	}
}

INLINE void test_bullet_collide( application& game, map_tile* billboard, const shader_program& singleColor )
{
    if ( game.bullet )
    {
        if( test_tile_collision( game,
                            game.bullet.get(),
                            game.bullet->mBody->position(),
                            ENTITY_BOUNDS_AIR_COLLIDE,
                            billboard,
                            ENTITY_BOUNDS_AIR_COLLIDE ) )
        {
            billboard->mColor = glm::vec4( 1.0f, 0.0f, 0.0f, 1.0f );

            game.billboard_oriented( *billboard, false );

            game.bullet->mBody->add_options( rigid_body::LOCK_INTEGRATION );

			if ( debug_flag_set() )
			{
                const obb& tileBox = *ENTITY_PTR_GET_BOX( billboard, ENTITY_BOUNDS_AIR_COLLIDE );

				singleColor.load_vec4( "color", glm::vec4( 1.0f ) );

				GL_CHECK( glPointSize( 50.0f ) );

				imm_draw d( singleColor );

				ray dbr;
				debug_get_ray( dbr );
				draw_debug_ray( game, d, tileBox, singleColor, dbr );

				GL_CHECK( glPointSize( 10.0f ) );

				game.bullet->mBody->add_options( rigid_body::LOCK_INTEGRATION );
			}
        }
        else if ( glm::distance( game.bullet->mBody->position(), billboard->mBody->position() ) <= 1.0f )
        {
            game.billboard_oriented( *billboard, false );

            game.bullet->mBody->add_options( rigid_body::LOCK_INTEGRATION );
        }
    }
}

void process_billboards( application& game, const view_data& vp, map_tile_list_t& billboards )
{
    const shader_program& singleColor = game.pipeline->programs().at( "single_color" );
    const shader_program& billboard = game.pipeline->programs().at( "billboard" );
    const draw_buffer& billboardBuffer = game.pipeline->draw_buffers().at( "billboard" );

    set_blend_mode blend( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    // Draw all billboards
    billboard.bind();
    billboard.load_mat4( "modelToView", vp.mTransform );
    billboard.load_mat4( "viewToClip", vp.mClipTransform );

	imm_draw d( singleColor );

    for ( map_tile* tile: billboards )
    {
        load_billboard_params( *tile, billboard  );

		const obb* tileBounds = ENTITY_PTR_GET_BOX( tile, ENTITY_BOUNDS_MOVE_COLLIDE );

        singleColor.bind();
        test_bullet_collide( game, tile, singleColor );
        billboard.bind();

        billboard.load_vec4( "color", tile->mColor );

        game.billTexture.bind( 0, "image", billboard );
        billboardBuffer.render( billboard );
		game.billTexture.release( 0 );

		if ( gTestFlags & DRAW_BILLBOARD_BOUNDS )
        {
            draw_billboard_bounds( game, vp, singleColor, *tileBounds );

			singleColor.load_mat4( "modelToView", vp.mTransform );

			halfspace hs( glm::mat3( tileBounds->axes() ), tileBounds->center(), 0.0f );
			singleColor.load_vec4( "color", glm::vec4( 0.0f, 0.0f, 1.0f, 1.0f ) );
			hs.draw( d );
        }

        billboard.bind();
    }
    billboard.release();
}

} // end namespace

static void draw_group( application& game,
                 const view_data& vp,
                 map_tile_list_t& billboards,
                 map_tile_list_t& walls,
                 map_tile_list_t& freeSpace )
{
    UNUSEDPARAM( freeSpace );

    const shader_program& singleColor = game.pipeline->programs().at( "single_color" );

    //const draw_buffer_t& billboardBuffer = game.pipeline->mDrawBuffers.at( "billboard" );

    // immDrawer can be used in some arbitrary code block that is aware of the renderer to draw something.
    // It's useful for debugging...
    imm_draw drawer( singleColor );
    gImmDrawer = &drawer;

    // Load a grey color so it looks somewhat fancy
    singleColor.bind();
    //singleColor.LoadVec4( "color", glm::vec4( 0.5f, 0.5f, 0.5f, 1.0f ) );

	if ( gTestFlags & DRAW_WALLS )
    {
        for ( const map_tile* tile: walls )
        {
            draw_bounds( game, *ENTITY_PTR_GET_BOX( tile, ENTITY_BOUNDS_AREA_EVAL ), glm::vec3( 0.5f ) );

			if ( gTestFlags & COLLIDE_WALLS )
			{
                test_tile_collision( game,
				 ( collision_entity::ptr_t )game.camera,
									vp.mOrigin,
									ENTITY_BOUNDS_MOVE_COLLIDE,
									tile,
									ENTITY_BOUNDS_MOVE_COLLIDE  );
			}
        }
    }

    if ( game.bullet )
    {
        draw_bounds( game, *( ENTITY_PTR_GET_BOX( game.bullet, ENTITY_BOUNDS_ALL ) ), glm::vec3( 1.0f, 0.0f, 0.0f ) );
    }

	if ( gTestFlags & DRAW_BILLBOARDS )
    {
        process_billboards( game, vp, billboards );
    }

    singleColor.bind();
    singleColor.load_mat4( "modelToView", vp.mTransform );
    singleColor.load_vec4( "color", glm::vec4( 0.0f, 1.0f, 0.0f, 1.0f ) );

	if ( gTestFlags & DRAW_HALFSPACES )
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
    draw_bounds( game, *( game.drawBounds ), glm::vec3( 1.0f, 0.0f, 1.0f ) );

    singleColor.bind();

	bool drawAdj = !!( gTestFlags & DRAW_REGIONS_ADJACENT );
	bool drawBounds = !!( gTestFlags & DRAW_REGIONS_BOUNDS );

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

    singleColor.release();
}

void exec_frame( void )
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

    game.camera->mViewParams.mInverseOrient = game.camera->mBody->orientation_mat4();

    const view_data& vp = game.camera->view_params();

    game.frustum.update( vp );

	if ( !game.drawAll )
	{
        game.gen->find_entities( game.billboards, game.walls, game.freeSpace, game.frustum, *( game.camera ) );
    };

    game.world.update( game );

    game.draw();

    // clear bodies which are added in world.Update call,
    // since we only want to integrate bodies which are in view
    game.world.mTime = get_time() - game.startTime;

    printf( "FPS: %f\r", 1.0f / game.world.mTime );
}

// temporary hack to get around the fact that querying for an game
// instance if an error is hgameening causes problems; we use this flag the exit.
static bool* runningPtr = nullptr;

uint32_t exec_game( void )
{   	
    application& game = application::get_instance();
	runningPtr = &game.running;

#ifdef EMSCRIPTEN
	InitEmInput();
#else
	while ( game.running )
    {
        exec_frame();

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
							gTestFlags ^= DRAW_BILLBOARDS;
                            break;

                        case SDLK_h:
							gTestFlags ^= DRAW_HALFSPACES;
                            break;

                        case SDLK_UP:
							gTestFlags ^= DRAW_CANCEL_REGIONS;
                            break;

                        default:
                            game.camera->eval_key_press( ( input_key ) e.key.keysym.sym );
                            break;
                    }
                    break;
                case SDL_KEYUP:
                    game.camera->eval_key_release( ( input_key ) e.key.keysym.sym );
                    break;
                case SDL_MOUSEMOTION:
					if ( !game.mouseShown )
                    {
                        game.camera->eval_mouse_move( ( float ) e.motion.xrel, ( float ) e.motion.yrel, false );
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
    return exec_game();
}

