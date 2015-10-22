#pragma once

#include "geom/geom.h"
#include "input.h"
#include "debug.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_main.h>
#include <memory>

template < typename child_t >
struct application_frame;

template < typename child_t >
struct application
{
private:
    std::vector< entity* > mFrameEntities;

protected:
    friend struct application_frame< child_t >;

    static typename std::unique_ptr< child_t > mInstance;

public:
    bool running = false;

    bool mouseShown = true;

    bool drawAll = false;

    SDL_Window* window = nullptr;

    SDL_Renderer* renderer = nullptr;

    SDL_GLContext context = nullptr;

    uint32_t width, height;

    float frameTime, lastTime, startTime;

    std::unique_ptr< render_pipeline > pipeline;

    input_client player, spec;

    input_client* camera;

    view_frustum frustum;

    application( uint32_t width, uint32_t height );

    virtual ~application( void );

    void toggle_culling( void );

    virtual void draw( void );

    virtual void frame( void ) = 0;

    virtual void update( void );

    virtual void handle_event( const SDL_Event& e );

    static child_t* instance( void );
};

template < typename child_t >
std::unique_ptr< child_t > application< child_t >::mInstance( nullptr );

template < typename child_t >
child_t* application< child_t >::instance( void )
{
    using ch_t = application< child_t >;

    if ( !ch_t::mInstance )
    {
        ch_t::mInstance.reset( new child_t( 1366, 768 ) );
    }

    return ch_t::mInstance.get();
}

template < typename child_t >
application< child_t >::application( uint32_t width_ , uint32_t height_ )
    : width( width_ ),
      height( height_ ),
      frameTime( 0.0f ),
      lastTime( 0.0f ),
      startTime( 0.0f ),
      camera( nullptr )
{
    SDL_Init( SDL_INIT_VIDEO );
    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, OP_GL_MAJOR_VERSION );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, OP_GL_MINOR_VERSION );

#ifndef OP_GL_USE_ES
    SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 24 );
    SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
    SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
    SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
    SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 8 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
#endif

    SDL_CreateWindowAndRenderer( width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN, &window, &renderer );
    context = SDL_GL_CreateContext( window );

    if ( !context )
    {
        MLOG_ERROR( "SDL_Error: %s", SDL_GetError() );
        return;
    }

#ifndef OP_GL_USE_ES
    glewExperimental = true;
    GLenum glewSuccess = glewInit();
    if ( glewSuccess != GLEW_OK )
    {
        MLOG_ERROR( "Could not initialize GLEW: %s", ( const char* ) glewGetErrorString( glewSuccess ) );
    }
#endif

    SDL_RendererInfo info;
    SDL_GetRendererInfo( renderer, &info );

    GL_CHECK( glClearColor( 0.0f, 0.0f, 0.0f, 1.0f ) );

    SDL_RenderPresent( renderer );

    pipeline.reset( new render_pipeline() );

    const shader_program& program = pipeline->programs().at( "single_color" );
    program.bind();

    spec.perspective( 60.0f, ( float ) width, ( float ) height, 0.1f, 10000.0f );
    player.perspective( 60.0f, ( float ) width, ( float ) height, 0.1f, 10000.0f );

    spec.mMode = input_client::MODE_SPEC;

    program.load_mat4( "viewToClip", player.view_params().mClipTransform );
    program.release();

    GL_CHECK( glEnable( GL_TEXTURE_2D ) );
    GL_CHECK( glDisable( GL_CULL_FACE ) );
    GL_CHECK( glEnable( GL_DEPTH_TEST ) );
    GL_CHECK( glDepthFunc( GL_LEQUAL ) );
    GL_CHECK( glClearDepthf( 1.0f ) );
    GL_CHECK( glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ) );

#ifndef OP_GL_USE_ES
    GL_CHECK( glPointSize( 10.0f ) );
#endif

    camera = &spec;
    running = true;
}

template < typename child_t >
application< child_t >::~application( void )
{
    SDL_Quit();
}

template < typename child_t >
void application< child_t >::toggle_culling( void )
{
    drawAll = !drawAll;
}

template < typename child_t >
void application< child_t >::draw( void )
{
    const view_data& vp = camera->view_params();

    debug_draw_axes( *this, vp );
    debug_draw_hud( *this );
}

template < typename child_t >
void application< child_t >::update( void )
{
    if ( camera )
    {
        frustum.update( camera->view_params() );
        camera->sync();
    }
}

template < typename child_t >
void application< child_t >::handle_event( const SDL_Event& e )
{
    switch ( e.type )
    {
        case SDL_KEYDOWN:
            switch ( e.key.keysym.sym )
            {
                case SDLK_ESCAPE:
                    SDL_SetRelativeMouseMode( SDL_FALSE );
                    running = false;
                    break;
                case SDLK_F1:
                    mouseShown = !mouseShown;
                    if ( mouseShown )
                    {
                        SDL_SetRelativeMouseMode( SDL_FALSE );
                    }
                    else
                    {
                        SDL_SetRelativeMouseMode( SDL_TRUE );
                    }
                    break;
                default:
                    camera->eval_key_press( ( input_key ) e.key.keysym.sym );
                    break;
            }
            break;
        case SDL_KEYUP:
            camera->eval_key_release( ( input_key ) e.key.keysym.sym );
            break;
        case SDL_MOUSEMOTION:
            if ( !mouseShown )
            {
                camera->eval_mouse_move( ( float ) e.motion.xrel, ( float ) e.motion.yrel, false );
            }
            break;
    }
}
