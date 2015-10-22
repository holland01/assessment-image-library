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

game::game( uint32_t width, uint32_t height )
    : game_app_t( width, height )
{
    billTexture.mip_map( true );
    billTexture.open_file( "asset/mooninite.png" );
    billTexture.load_2d();
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

void game::update( void )
{
    game_app_t::update();

    mWorld.step();
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
