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
        COLLIDE_WALLS = 1 << 9,
        DRAW_REGIONS_WALLS = 1 << 10
    };

    const float DISTANCE_THRESHOLD = 2.0f;

	std::unordered_map< std::string, uint32_t > gTestConfig =
    {
        { "adjacency_test", DRAW_REGIONS_ADJACENT | DRAW_WALLS | DRAW_REGIONS_BOUNDS },
        { "default", DRAW_BILLBOARDS | DRAW_WALLS | DRAW_HALFSPACES },
        { "region_tiles_test", DRAW_REGIONS_BOUNDS | DRAW_WALLS | DRAW_REGIONS_ADJACENT },
        { "region_wall_tiles_test", DRAW_WALLS | DRAW_REGIONS_WALLS }
    };

    uint32_t gTestFlags = gTestConfig[ "region_wall_tiles_test" ];
}

game::game( uint32_t width, uint32_t height )
    : game_app_t( width, height )
{
    billTexture.mip_map( true );
    billTexture.open_file( "asset/mooninite.png" );
    billTexture.load_2d();

    camera = &player;
    camera->position( glm::vec3( 0.0f ) );
    camera->set_physics( 80.0f, glm::translate( glm::mat4( 1.0f ), camera->position() ) );

    spec.set_physics( 80.0f, glm::translate( glm::mat4( 1.0f ), glm::vec3( 0.0f, 10.0f, 0.0f ) ) );
    spec.toggle_kinematic( false );
    spec.flip_move_state();
    spec.move_step( 1.0f );
}

void game::fill_orient_map( void )
{
    mBillboardsOriented.clear();

    // TODO: iterate through billboards heres
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

    draw();

    mWorld.clear_physics_entities();

    printf( "Position: %s\n", glm::to_string( camera->position() ).c_str() );
}

void game::clear_entities( std::vector< entity* >& list )
{
    for ( entity* e: list )
    {
        e->remove_from_world();
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
        UNUSEDPARAM( g );
        // TODO: orient billboards here
    }

}

void game::fill_entities( std::vector< entity* >& list ) const
{
    // kinda sorta shouldn't be called from here, but whatever - it works for now.
    update_billboards( *this );

    auto add_if_alive = [ &list, this ]( entity* e ) -> void
    {
        if ( e )
        {
            list.push_back( e );
            e->add_to_world();
        }
    };

    add_if_alive( camera );
}

void game::draw( void )
{
    const view_data& vp = camera->view_params();

    UNUSEDPARAM( vp );


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

}
