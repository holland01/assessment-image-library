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

#define VERT( p, c ) { p, glm::vec3( 0.0f ), glm::vec2( 0.0f ), c }

using namespace rend;

static shader_program_t MakeProg( void )
{
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

    shader_program_t prog( vshader, fshader, {}, { "position", "color" } );
    return std::move( prog );
}

struct app_t
{
    bool running = false;
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_GLContext context = nullptr;

    uint32_t width, height;

    shader_program_t program;

    GLuint vbo = 0;

    app_t( uint32_t width, uint32_t height );
   ~app_t( void );

    void Frame( void );
    uint32_t Exec( void );
};

app_t::app_t( uint32_t width_ , uint32_t height_ )
    : width( width_ ),
      height( height_ )
{
    SDL_Init( SDL_INIT_VIDEO );

    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 2 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 1 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG );

    SDL_CreateWindowAndRenderer( width, height, SDL_WINDOW_OPENGL, &window, &renderer );
    context = SDL_GL_CreateContext( window );

    glClearColor( 1.0f, 0.0f, 0.0f, 1.0f );

    SDL_RenderPresent( renderer );

    std::array< geom::draw_vertex_t, 3 > vertices =
    {{
        VERT( glm::vec3( -1.0f, 0.0f, 0.0f ), glm::u8vec4( 255, 0, 0, 255 ) ),
        VERT( glm::vec3( 0.0f, 1.0f, 0.0f ), glm::u8vec4( 0, 255, 0, 255 ) ),
        VERT( glm::vec3( 1.0f, 0.0f, 0.0f ), glm::u8vec4( 0, 0, 255, 255 ) )
    }};

    GL_CHECK( glGenBuffers( 1, &vbo ) );
    GL_CHECK( glBindBuffer( GL_ARRAY_BUFFER, vbo ) );
    GL_CHECK( glBufferData( GL_ARRAY_BUFFER,
        sizeof( vertices[ 0 ] ) * 3, &vertices[ 0 ], GL_STATIC_DRAW ) );

    program = MakeProg();

    shader_program_t::LoadAttribLayout< geom::draw_vertex_t >( program );

    program.Bind();

    running = true;
}

app_t::~app_t( void )
{
    SDL_Quit();
}

void app_t::Frame( void )
{
    GL_CHECK( glClear( GL_COLOR_BUFFER_BIT ) );
    GL_CHECK( glDrawArrays( GL_TRIANGLES, 0, 3 ) );
}

uint32_t app_t::Exec( void )
{
#ifdef EMSCRIPTEN
    emscripten_set_main_loop( ( em_callback_func )LoopIter, 0, 1 );
#else
    while ( running )
    {
        Frame();

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

    return 0;
}

static app_t* appPtr = nullptr;

void FlagExit( void )
{
#ifdef EMSCRIPTEN
    exit( 1 );
#else
    if ( appPtr )
    {
        appPtr->running = false;
    }
#endif
}

int main( void ) 
{
    app_t app( 800, 600 );

    appPtr = &app;

    return app.Exec();
}

