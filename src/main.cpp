//#include "def.h"

#include <iostream>
#ifdef EMSCRIPTEN
#   include <emscripten.h>
#   include <html5.h>
#endif

#include <SDL2/SDL.h>
#include <SDL2/SDL_main.h>

#include "renderer.h"
#include "geom.h"
#include "view.h"

#include <unistd.h>
#include <stdint.h>
#include <array>
#include <cstdlib>
#include <unordered_map>

#define VERT( p, c ) { p, glm::vec3( 0.0f ), glm::vec2( 0.0f ), c }

static rend::shader_program_t MakeProg( void )
{
    std::string vshader( GEN_V_SHADER(
        attribute vec3 position;
        attribute vec4 color;

        varying vec4 frag_Color;

        uniform mat4 modelToView;
        uniform mat4 viewToClip;

        void main( void )
        {
            gl_Position = viewToClip * modelToView * vec4( position, 1.0 );
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

    rend::shader_program_t prog( vshader, fshader, { "modelToView", "viewToClip" }, { "position", "color" } );
    return std::move( prog );
}

struct app_t
{
    bool running = false;
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_GLContext context = nullptr;

    uint32_t width, height;

    rend::shader_program_t program;

    view::camera_t         camera;

    GLuint vbo = 0;

    app_t( uint32_t width, uint32_t height );
   ~app_t( void );

    static app_t& GetInstance( void );
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

    GL_CHECK( glClearColor( 1.0f, 0.0f, 0.0f, 1.0f ) );

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

    rend::shader_program_t::LoadAttribLayout< geom::draw_vertex_t >( program );

    program.Bind();

    camera.SetViewOrigin( glm::vec3( 0.0f, 0.0f, 5.0f ) );
    camera.SetPerspective( 60.0f, ( float ) width, ( float ) height, 0.1f, 100.0f );
    program.LoadMat4( "viewToClip", camera.GetViewParams().clipTransform );

    running = true;
}

app_t::~app_t( void )
{
    SDL_Quit();
}

app_t& app_t::GetInstance( void )
{
    static app_t app( 800, 600 );
    return app;
}

namespace {

#ifdef EMSCRIPTEN

std::unordered_map< std::string, input_key_t > emKeyMap =
{
    { "KeyW", input_key_t::W },
    { "KeyS", input_key_t::S },
    { "KeyA", input_key_t::A },
    { "KeyD", input_key_t::D },
    { "KeyE", input_key_t::E },
    { "KeyQ", input_key_t::Q },
    { "Escape", input_key_t::ESC },
    { "Space", input_key_t::SPACE }
};

EM_BOOL EM_KeyDown( int32_t eventType, const EmscriptenKeyboardEvent *keyEvent, void *userData )
{
    UNUSEDPARAM( eventType );
    UNUSEDPARAM( userData );

    printf( "EMCode: %s, EMKey: %s\n", keyEvent->code, keyEvent->key );

    app_t& app = app_t::GetInstance();

    if ( keyEvent->shiftKey )
    {
        app.camera.EvalKeyPress( input_key_t::LSHIFT );
    }
    else
    {
        auto entry = emKeyMap.find( keyEvent->code );

        if ( entry == emKeyMap.end() )
        {
            return 0;
        }

        if ( entry->second == input_key_t::ESC )
        {
            app.running = false;
        }
        else
        {
            app.camera.EvalKeyPress( entry->second );
        }
    }

    return 1;
}
#endif // EMSCRIPTEN

void App_Frame( void )
{
    app_t& app = app_t::GetInstance();

#ifdef EMSCRIPTEN
    if ( !app.running )
    {
        emscripten_cancel_main_loop();
        std::exit( 0 );
    }
#endif

    app.camera.Update();
    app.program.LoadMat4( "modelToView", app.camera.GetViewParams().transform );

    GL_CHECK( glClear( GL_COLOR_BUFFER_BIT ) );
    GL_CHECK( glDrawArrays( GL_TRIANGLES, 0, 3 ) );
}

static uint32_t App_Exec( void )
{   
    app_t& app = app_t::GetInstance();

#ifdef EMSCRIPTEN
    int32_t ret = emscripten_set_keypress_callback( nullptr, nullptr, 0, ( em_key_callback_func )&EM_KeyDown );
    if ( ret != EMSCRIPTEN_RESULT_SUCCESS )
    {
        MLOG_ERROR( "Fuck shit balls" );
    }

    MLOG_WARNING( "Test" );

    emscripten_set_main_loop( ( em_callback_func )&App_Frame, 0, 1 );
#else
    while ( app.running )
    {
        App_Frame();

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
                    app.camera.EvalKeyPress( ( input_key_t ) e.key.keysym.sym );
                    break;
                case SDL_KEYUP:
                    app.camera.EvalKeyRelease( ( input_key_t ) e.key.keysym.sym );
                    break;
            }
        }
    }
#endif

    return 0;
}

}

void FlagExit( void )
{
    app_t::GetInstance().running = false;
}

int main( void ) 
{
    return App_Exec();
}

