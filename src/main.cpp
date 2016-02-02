#include "test/game.h"
#include "test/delaunay.h"
#include "test/image.h"

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
	return application< image_test >::run();
}

