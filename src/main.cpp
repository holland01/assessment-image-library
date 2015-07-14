#include "def.h"

#include <iostream>
#include <emscripten.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_main.h>

#include OPENGL_API_H
#include OPENGL_API_EXT_H

#include <unistd.h>
#include <stdint.h>

uint32_t mask_r = 0x000000ff;
uint32_t mask_g = 0x0000ff00;
uint32_t mask_b = 0x00ff0000;
uint32_t mask_a = 0xff000000;

#ifndef EMSCRIPTEN
#	define EMSCRIPTEN
#endif

static SDL_Renderer* renderer = NULL;

static void loop_iter(void) {
	glClear(GL_COLOR_BUFFER_BIT);	
}

static inline void run_loop(void) {
#ifdef EMSCRIPTEN
	emscripten_set_main_loop((em_callback_func)loop_iter, 0, 1);
#else
	#error "Native code?! Fuck that shit. Use emscripten!"
#endif
}

int main(void) {
    SDL_Init(SDL_INIT_VIDEO);
   
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 2 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 0 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG );
    SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, 8 );
 
	SDL_Window *window;
	SDL_RendererInfo info;
	SDL_GLContext gl_context;

    SDL_CreateWindowAndRenderer(256, 256, SDL_WINDOW_OPENGL, &window, &renderer);
	SDL_GetRendererInfo(renderer, &info);
	gl_context = SDL_GL_CreateContext(window);	
	 
	SDL_Surface *screen = SDL_CreateRGBSurface(0, 256, 256, 32, mask_r, mask_g, mask_b, mask_a);
    
	if (SDL_MUSTLOCK(screen)) SDL_LockSurface(screen);
    if (SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
   
	glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
 
	SDL_RenderPresent(renderer);
    
	run_loop();
	
	SDL_Quit();

   	return 0;
}

