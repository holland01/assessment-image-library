#include "application.h"
#include "game.h"
#include "physics_simulation.h"
#include "debug_app.h"

// temporary hack to get around the fact that querying for an game
// instance if an error is happening causes problems; we use this flag the exit.
static bool* runningPtr = nullptr;

template < typename child_t >
uint32_t run( void )
{
    using app_t = application< child_t >;

    child_t* app = app_t::instance();
    runningPtr = &app->running;

#ifdef EMSCRIPTEN
    InitEmInput();
#else
    while ( app->running )
    {
        GL_CHECK( glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ) );

        app->frame();

        SDL_GL_SwapWindow( app->window );

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
	return run< game >();
}

