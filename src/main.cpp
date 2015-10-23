#include "application.h"
#include "test/game.h"
#include "debug_app.h"

bool gRunning = true;

template < typename child_t >
uint32_t run( void )
{
    using app_t = application< child_t >;

    child_t* app = app_t::instance();

#ifdef EMSCRIPTEN
    InitEmInput();
#else
    while ( gRunning )
    {
        GL_CHECK( glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ) );

        app->frame();

        SDL_GL_SwapWindow( app->window_handle() );

        SDL_Event e;
        while ( SDL_PollEvent( &e ) )
        {
            app->handle_event( e );
        }
    }
#endif

    return 0;
}

void flag_exit( void )
{
    gRunning = false;
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
    return run< game >();
}

