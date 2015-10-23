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
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/projection.hpp>
#include <glm/gtx/string_cast.hpp>

#include "debug_app.h"

void exec_frame( void );

#include "eminput.h"

unsigned int gDebugFlag = 0;

game::game( uint32_t width, uint32_t height )
    : game_app_t( width, height )
{
    mBillTexture.mip_map( true );
    mBillTexture.open_file( "asset/mooninite.png" );
    mBillTexture.load_2d();
}

void game::fill_orient_map( void )
{
    mBillboardsOriented.clear();

    // TODO: iterate through billboards heres
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

    printf( "Position: %s\n", glm::to_string( mCamPtr->position() ).c_str() );
}

void game::update( void )
{
    game_app_t::update();

    mWorld.step();
}

void game::draw( void )
{
    const view_data& vp = mCamPtr->view_params();

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
                    if ( mCamPtr == &mPlayer )
                    {
                        mCamPtr = &mSpec;
                    }
                    else
                    {
                        mCamPtr = &mPlayer;
                    }
                    break;
            }
            break;
    }
}
