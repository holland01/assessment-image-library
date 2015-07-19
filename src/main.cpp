//#include "def.h"

#include <iostream>
#ifdef EMSCRIPTEN
#   include <emscripten.h>
#endif

#include <SDL2/SDL.h>
#include <SDL2/SDL_main.h>

#include "renderer.h"
#include "geom.h"

#include <unistd.h>
#include <stdint.h>
#include <array>

using namespace glrend;

static bool running = true;
static SDL_Window* window = nullptr;


#define VERT( p, c ) { p, glm::vec3( 0.0f ), glm::vec2( 0.0f ), c }

static void LoopIter( void )
{
    GL_CHECK( glClear( GL_COLOR_BUFFER_BIT ) );
    GL_CHECK( glDrawArrays( GL_TRIANGLES, 0, 3 ) );
}

static inline void RunLoop( void )
{
#ifdef EMSCRIPTEN
    emscripten_set_main_loop( ( em_callback_func )LoopIter, 0, 1 );
#else
    while ( running )
    {
        LoopIter();

        SDL_GL_SwapWindow( window );

        SDL_Event e;
        while ( SDL_PollEvent( &e ) )
        {
            switch ( e.type )
            {
                case SDL_KEYDOWN:
                    if ( e.key.keysym.sym == SDLK_ESCAPE )
                    {
                        running = false;
                    }
                    break;
            }
        }
    }
#endif
}

void FlagExit( void )
{
#ifdef EMSCRIPTEN
    exit( 1 );
#else
    running = false;
#endif
}

int main( void ) 
{
    SDL_Init( SDL_INIT_VIDEO );
   
    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 2 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 0 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG );
    SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, 8 );
 
    SDL_Renderer* renderer = nullptr;
    SDL_RendererInfo info;
    SDL_GLContext gl_context;

    SDL_CreateWindowAndRenderer( 512, 512, SDL_WINDOW_OPENGL, &window, &renderer );
    SDL_GetRendererInfo( renderer, &info );
    gl_context = SDL_GL_CreateContext(window);

    glClearColor( 1.0f, 0.0f, 0.0f, 1.0f );

    SDL_RenderPresent(renderer);

    std::array< geom::drawVertex_t, 3 > vertices =
    {{
        VERT( glm::vec3( -1.0f, 0.0f, 0.0f ), glm::u8vec4( 255, 0, 0, 255 ) ),
        VERT( glm::vec3( 0.0f, 1.0f, 0.0f ), glm::u8vec4( 0, 255, 0, 255 ) ),
        VERT( glm::vec3( 1.0f, 0.0f, 0.0f ), glm::u8vec4( 0, 0, 255, 255 ) )
    }};

    std::string vshader( GEN_V_SHADER(
        attribute vec3 position;
        attribute vec4 color;

        varying vec4 frag_Color;

        void main( void )
        {
            gl_Position = vec4( position, 1.0 );
            frag_Color = color;
        }
    ) );

    std::string fshader( GEN_F_SHADER(
        varying vec4 frag_Color;
        void main( void )
        {
            gl_FragColor = frag_Color;
        }
    ) );

    Program prog( vshader, fshader, {}, { "position", "color" } );

    GLuint vbo;
    GL_CHECK( glGenBuffers( 1, &vbo ) );
    GL_CHECK( glBindBuffer( GL_ARRAY_BUFFER, vbo ) );
    GL_CHECK( glBufferData( GL_ARRAY_BUFFER,
        sizeof( vertices[ 0 ] ) * 3, &vertices[ 0 ], GL_STATIC_DRAW ) );

    Program::LoadAttribLayout< geom::drawVertex_t >( prog );

    prog.Bind();

    RunLoop();

    SDL_Quit();

    return 0;
}

