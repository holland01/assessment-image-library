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
#include <glm/gtx/projection.hpp>
#include <glm/gtc/quaternion.hpp>

#include "debug_app.h"

#include "application_update.h"

void exec_frame( void );

#include "eminput.h"

unsigned int gDebugFlag = 0;

static void draw_group( game& game,
                 const view_data& vp,
                 map_tile_list_t& billboards,
                 map_tile_list_t& walls,
                 map_tile_list_t& freeSpace );

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
        { "default", DRAW_BILLBOARDS | DRAW_WALLS | DRAW_HALFSPACES },
        { "bounds_tiles_test", DRAW_REGIONS_BOUNDS | DRAW_CANCEL_REGIONS }
    };

    uint32_t gTestFlags = gTestConfig[ "default" ];
}

game::game( uint32_t width, uint32_t height )
    : game_app_t( width, height )
{
    billTexture.mip_map( true );
    billTexture.open_file( "asset/mooninite.png" );
    billTexture.load_2d();

    const map_tile* startTile = reset_map();
    camera = &player;
    camera->position( gen->scale_to_world( glm::vec3( startTile->mX, 0.0f, startTile->mZ ) ) );
    camera->set_physics( 80.0f, glm::translate( glm::mat4( 1.0f ), camera->position() ) );
}

void game::fill_orient_map( void )
{
    mBillboardsOriented.clear();

    for ( const map_tile* b: gen->mBillboards )
    {
        billboard_oriented( *b, true );
    }
}

const map_tile* game::reset_map( void )
{
    gen.reset( new map_tile_generator() );
    fill_orient_map();

    std::sort( gen->mFreeSpace.begin(), gen->mFreeSpace.end(), []( const map_tile* a, const map_tile* b ) -> bool
    {
        glm::vec2 va( a->mX, a->mZ ), vb( b->mX, b->mZ );

        return glm::length( va ) < glm::length( vb );
    });

    return gen->mFreeSpace[ gen->mFreeSpace.size() / 2 ];
}

void game::fire_gun( void )
{
    debug_set_flag( false );
    bullet.reset( new entity() );

    bullet->mSize = glm::vec3( 0.1f );

    // TODO: add physics here
}

namespace {
    game* game_ptr( void )
    {
        return ( game* )( game_app_t::instance() );
    }
}

void game::frame( void )
{
    update();

//!!FIXME: throw this somewhere more logical, like have it managed by the application base class
#ifdef EMSCRIPTEN
    if ( !running )
    {
        emscripten_cancel_main_loop();
        std::exit( 0 );
    }
#endif

    if ( !drawAll )
    {
        gen->find_entities( billboards, walls, freeSpace, frustum, *( camera ) );
    };

    draw();

    mWorld.clear_physics_entities();
}

void game::clear_entities( std::vector< entity* >& list )
{
    for ( entity* e: list )
    {
        e->normal_body().remove_from_world();
    }

    game_app_t::clear_entities( list );
}

void game::update( void )
{
    game_app_t::update();

    mWorld.step();
}

namespace {
    INLINE void update_billboards( const game& g )
    {
        map_tile_list_t billboards = std::move( g.billboard_list() );

        for ( map_tile* t: billboards )
        {
            if ( t )
            {
                if ( g.billboard_oriented( *t ) )
                {
                    t->orient_kinematic_to( g.camera->view_params().mOrigin );
                }
            }
        }
    }

}

void game::fill_entities( std::vector< entity* >& list ) const
{
    // kinda sorta shouldn't be called from here, but whatever - it works for now.
    update_billboards( *this );

    auto add_to_list = [ &list, this ]( map_tile_list_t& mapTileList ) -> void
    {
        for ( entity* e: mapTileList )
        {
            list.push_back( e );
            e->normal_body().add_to_world();
        }
    };

    auto add_if_alive = [ &list, this ]( entity* e ) -> void
    {
        if ( e )
        {
            list.push_back( e );
            e->normal_body().add_to_world();
        }
    };

    add_if_alive( camera );

    map_tile_list_t walllist( std::move( wall_list() ) );
    map_tile_list_t billboardlist( std::move( billboard_list() ) );

    list.reserve( walllist.size() + billboardlist.size() + 1 );
    add_to_list( walllist );
    add_to_list( billboardlist );
}

void game::draw( void )
{
    const view_data& vp = camera->view_params();

    if ( drawAll )
    {
        draw_group( *this, vp, gen->mBillboards, gen->mWalls, gen->mFreeSpace );
    }
    else
    {
        draw_group( *this, vp, billboards, walls, freeSpace );
    }

    application< game >::draw();
}

void game::handle_event( const SDL_Event& e )
{
    application< game >::handle_event( e );

    switch ( e.type )
    {
        case SDL_KEYDOWN:
            switch ( e.key.keysym.sym )
            {
                case SDLK_r:
                    reset_map();
                    break;
                case SDLK_v:
                    toggle_culling();
                    break;
                case SDLK_c:
                    if ( camera == &player )
                    {
                        camera = &spec;
                    }
                    else
                    {
                        camera = &player;
                    }
                    break;
                case SDLK_b:
                    gTestFlags ^= DRAW_BILLBOARDS;
                    break;

                case SDLK_h:
					gTestFlags ^= DRAW_WALLS;
                    break;

                case SDLK_UP:
                    gTestFlags ^= DRAW_CANCEL_REGIONS;
                    break;
            }
            break;

        case SDL_MOUSEBUTTONDOWN:
            if ( e.button.button == SDL_BUTTON_LEFT )
            {
                fire_gun();
            }
            break;
    }
}

namespace {

uint32_t regionIter = 0;
float frameCount = 0.0f;

// For coloring arbitrary amounts of tiles
glm::mat4 gQuadTransform(
    []( void ) -> glm::mat4
    {
        // Rotate 90 degrees in the x-axis so the quad is on the ground, and then set the y-axis to -1
        // so that it's on the same level as the bottom of the bounding boxes ( i.e., walls )
        glm::mat4 t( glm::rotate( glm::mat4( 1.0f ), glm::half_pi< float >(), glm::vec3( 1.0f, 0.0f, 0.0f ) ) );
        t[ 3 ].y = -1.0f;
        return t;
    }()
);

INLINE void draw_tiles( const game_app_t& game,
                        const std::vector< const map_tile* >& tiles,
                        const glm::vec3& color, float alpha = 1.0f )
{
    for ( const map_tile* tile: tiles )
    {
        tile->normal_body().draw( "colored_cube", "single_color",
                                   game.camera->view_params(),
                                   glm::vec4( color, alpha ) );
    }
}

INLINE void draw_adjacent_tiles( const game_app_t& game, const adjacent_region_list_t& adjRegions )
{
    for ( const adjacent_region& br: adjRegions )
    {
        auto adj = br.mRegion.lock();

        if ( !adj )
            continue;

        draw_tiles( game, adj->mTiles, glm::vec3( adj->mColor ), 1.0f );
    }
}

INLINE bool in_adjacent_region_list( ref_tile_region_t source, const adjacent_region_list_t& adjRegions )
{
    for ( const adjacent_region& r: adjRegions )
        if ( source == r.mRegion )
            return true;

    return false;
}

bool has_cancel_regions( void )
{
	return !!( gTestFlags & DRAW_CANCEL_REGIONS );
}

void draw_regions( game& g, bool drawAdjacent = false )
{  
    for ( uint32_t i = 0; i < g.gen->mRegions.size(); ++i )
    {
        ref_tile_region_t weakRegion = g.gen->mRegions[ i ];
        auto region = weakRegion.lock();
        if ( !region )
            continue;

        bool canDraw = g.gen->mRegions[ regionIter ] != region
                && !in_adjacent_region_list( weakRegion, g.gen->mRegions[ regionIter ]->mAdjacent )
                && !has_cancel_regions();

        // If drawAdjacent is turned on, then we cannot draw
        // the regions as normal if i == regionIter: for some reason,
        // despite being rendered previously, this pass will overwrite
        // following draw pass in the color buffer, which produces inaccurate results.
        if ( canDraw )
            draw_tiles( g, region->mTiles, glm::vec3( region->mColor ), region->mColor.a );

        if ( i == regionIter && drawAdjacent )
            draw_adjacent_tiles( g, region->mAdjacent );
    }
}

INLINE void load_billboard_params( map_tile& tile, const shader_program& billboard )
{
    const glm::mat4 wT( tile.kinematic_body().world_transform() );

    glm::vec3 boundsOrigin( wT[ 3 ] );

    billboard.load_vec3( "origin", boundsOrigin );

    billboard.load_mat3( "viewOrient", glm::mat3( wT ) );
}

void process_billboards( game& game, const view_data& vp, map_tile_list_t& billboards )
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

        singleColor.bind();
        billboard.bind();

        billboard.load_vec4( "color", tile->mColor );

        game.billTexture.bind( 0, "image", billboard );
        billboardBuffer.render( billboard );
		game.billTexture.release( 0 );

        // Draw forward direction
        {
            btTransform wT;
            tile->kinematic_body().motion_state().getWorldTransform( wT );
            const btMatrix3x3& orientation = wT.getBasis();

            d.begin( GL_LINES );
            d.vertex( glm::vec3( 0.0f ) );
            d.vertex( glm::ext::from_bullet( orientation[ 2 ] ) );
            d.end();
        }
        if ( gTestFlags & DRAW_BILLBOARD_BOUNDS )
            tile->normal_body().draw( "colored_cube", "single_color", game.camera->view_params(), tile->mColor );

        billboard.bind();
    }
    billboard.release();
}

} // end namespace

static void draw_group( game& game,
                 const view_data& vp,
                 map_tile_list_t& billboards,
                 map_tile_list_t& walls,
                 map_tile_list_t& freeSpace )
{
    UNUSEDPARAM( freeSpace );

    const shader_program& singleColor = game.pipeline->programs().at( "single_color" );

    // immDrawer can be used in some arbitrary code block that is aware of the renderer to draw something.
    // It's useful for debugging...
    imm_draw drawer( singleColor );
    gImmDrawer = &drawer;

    // Load a grey color so it looks somewhat fancy
    singleColor.bind();

	if ( gTestFlags & DRAW_WALLS )
    {
        for ( const map_tile* tile: walls )
        {
            tile->normal_body().draw( "colored_cube", "single_color", game.camera->view_params(), glm::vec4( 0.5f, 0.5f, 0.5f, 1.0f ) );
        }
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
                continue;

            const map_tile_generator::collision_face_table_t& table =
                    game.gen->wall_surf_table( wall->mHalfSpaceIndex );

            for ( int32_t i: table )
            {
                if ( i < 0 )
                {
                    continue;
                }

                game.gen->wall_surf( i ).draw( drawer );
            }
        }
    }

    singleColor.bind();

    if ( gTestFlags & DRAW_REGIONS_BOUNDS )
    {
        draw_regions( game, !!( gTestFlags & DRAW_REGIONS_ADJACENT ) );
    }

    frameCount += 1.0f;

    if ( regionIter == game.gen->mRegions.size() )
    {
        regionIter = 0;
    }

    singleColor.release();
}
