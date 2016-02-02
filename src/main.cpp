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
	/*
    uint8_t num[4] = { 0x00, 0x00, 0x80, 0xBF };

    float x = *((float*)num);
	uint32_t y = *((uint32_t*)&x);

	uint8_t* bx = (uint8_t*)&x;

	printf( "as float: %f, as uint32_t: %x, last byte: %x\n", x, y, bx[ 3 ] );

    return 0;
	*/

	//printf("Value: %lu\n", img::rgb_f24_t::PIXEL_STRIDE_BYTES);
	//return 0;

	return application< image_test >::run();
}

