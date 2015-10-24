#pragma once

#include "geom/geom.h"
#include "input.h"
#include "debug.h"
#include "messenger.h"

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

    bool mRunning = false;

    bool mMouseShown = true;

    bool mDrawAll = false;

    SDL_Window* mWindow = nullptr;

    SDL_Renderer* mRenderer = nullptr;

    SDL_GLContext mContext = nullptr;

    uint32_t mWidth, mHeight;

    float mFrameTime, mLastTime, mStartTime;

    std::unique_ptr< render_pipeline > mPipeline;

    input_client mPlayer, mSpec;

    input_client* mCamPtr;

    view_frustum mFrustum;

public:

    application( uint32_t mWidth, uint32_t mHeight );

    virtual ~application( void );

    void toggle_culling( void );

    virtual void draw( void );

    virtual void frame( void ) = 0;

    virtual void update( void );

    void quit( void );

    virtual void handle_event( const SDL_Event& e );

    SDL_Window* window_handle( void ) { return mWindow; }

    bool running( void ) const { return mRunning; }

    const render_pipeline& pipeline( void ) const { return *( mPipeline.get() ); }

    static child_t* instance( void );

    static int32_t run( void );
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
int32_t application< child_t>::run( void )
{
    using app_t = application< child_t >;

    child_t* app = app_t::instance();

#ifdef EMSCRIPTEN
    InitEmInput();
#else
    while ( app->running() )
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

template < typename child_t >
application< child_t >::application( uint32_t width_ , uint32_t height_ )
    : mWidth( width_ ),
      mHeight( height_ ),
      mFrameTime( 0.0f ),
      mLastTime( 0.0f ),
      mStartTime( 0.0f ),
      mCamPtr( nullptr )
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

    SDL_CreateWindowAndRenderer( mWidth, mHeight, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN, &mWindow, &mRenderer );
    mContext = SDL_GL_CreateContext( mWindow );

    if ( !mContext )
    {
        MLOG_ERROR( "SDL_Error: %s", SDL_GetError() );
        return;
    }

#ifndef OP_GL_USE_ES
    glewExperimental = true;
    GLenum glewSuccess = glewInit();
    if ( glewSuccess != GLEW_OK )
        MLOG_ERROR( "Could not initialize GLEW: %s", ( const char* ) glewGetErrorString( glewSuccess ) );
#endif

    SDL_RendererInfo info;
    SDL_GetRendererInfo( mRenderer, &info );

    GL_CHECK( glClearColor( 0.0f, 0.0f, 0.0f, 1.0f ) );

    SDL_RenderPresent( mRenderer );

    mPipeline.reset( new render_pipeline() );

    std::vector< std::string > perspectiveLoad = { "vertex_color", "single_color" };

    mSpec.perspective( 60.0f, ( float ) mWidth, ( float ) mHeight, 0.1f, 10000.0f );
    mPlayer.perspective( 60.0f, ( float ) mWidth, ( float ) mHeight, 0.1f, 10000.0f );

    mSpec.mMode = input_client::spectate;

    for ( const auto& name: perspectiveLoad )
    {
        bind_program p( name );
        p.program().load_mat4( "viewToClip", mPlayer.view_params().mClipTransform );
    }

    GL_CHECK( glEnable( GL_TEXTURE_2D ) );
    GL_CHECK( glDisable( GL_CULL_FACE ) );
    GL_CHECK( glEnable( GL_DEPTH_TEST ) );
    GL_CHECK( glDepthFunc( GL_LEQUAL ) );
    GL_CHECK( glClearDepthf( 1.0f ) );
    GL_CHECK( glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ) );

#ifndef OP_GL_USE_ES
    GL_CHECK( glPointSize( 10.0f ) );
#endif

    mCamPtr = &mSpec;
    mRunning = true;
}

template < typename child_t >
application< child_t >::~application( void )
{
    SDL_Quit();
}

template < typename child_t >
void application< child_t >::toggle_culling( void )
{
    mDrawAll = !mDrawAll;
}

template < typename child_t >
void application< child_t >::draw( void )
{
    if ( mCamPtr )
    {
        const view_data& vp = mCamPtr->view_params();

        debug_draw_axes( *this, vp );
        debug_draw_hud( *this );
    }
}

template < typename child_t >
void application< child_t >::update( void )
{
    if ( mCamPtr )
    {
        mFrustum.update( mCamPtr->view_params() );
        mCamPtr->sync();
    }
}

template < typename child_t >
void application< child_t >::quit( void )
{
    if ( mCamPtr )
    {
        SDL_SetRelativeMouseMode( SDL_FALSE );
        mRunning = false;
        return;
    }
}

template < typename child_t >
void application< child_t >::handle_event( const SDL_Event& e )
{
    if ( gMessenger.mQuit )
    {
        quit();
        return;
    }

    switch ( e.type )
    {
        case SDL_KEYDOWN:
            switch ( e.key.keysym.sym )
            {
                case SDLK_ESCAPE:
                    quit();
                    break;
                case SDLK_F1:
                    mMouseShown = !mMouseShown;
                    if ( mMouseShown )
                        SDL_SetRelativeMouseMode( SDL_FALSE );
                    else
                        SDL_SetRelativeMouseMode( SDL_TRUE );
                    break;
                case SDLK_l:
                    if ( mCamPtr )
                        mCamPtr->flags( mCamPtr->flags() ^ input_client::flags::lock_orientation );
                    break;
                default:
                    if ( mCamPtr )
                        mCamPtr->eval_key_press( ( input_key ) e.key.keysym.sym );
                    break;

            }
            break;
        case SDL_KEYUP:
            mCamPtr->eval_key_release( ( input_key ) e.key.keysym.sym );
            break;
        case SDL_MOUSEMOTION:
            if ( !mMouseShown )
            {
                mCamPtr->eval_mouse_move( ( float ) e.motion.xrel, ( float ) e.motion.yrel, false );
            }
            break;
    }
}
