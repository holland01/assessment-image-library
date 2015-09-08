#include "application.h"

application::application( uint32_t width_ , uint32_t height_ )
    : width( width_ ),
      height( height_ ),
      frameTime( 0.0f ),
      lastTime( 0.0f ),
      startTime( 0.0f ),
      camera( nullptr ),
      drawBounds( nullptr ),
      world( 1.0f, OP_PHYSICS_DT )
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

    camera = &player;
    drawBounds = spec.query_bounds( ENTITY_BOUNDS_MOVE_COLLIDE )->to_box();

    running = true;
}

application::~application( void )
{
    SDL_Quit();
}

application& application::get_instance( void )
{
    static application game( 1366, 768 );
    return game;
}

void application::make_body( input_client &dest, input_client::client_mode mode, const glm::vec3 &pos, float mass )
{
    uint32_t flags = rigid_body::RESET_VELOCITY | rigid_body::RESET_FORCE_ACCUM | rigid_body::RESET_TORQUE_ACCUM;

    rigid_body* body = new rigid_body( flags );
    body->position( pos );
    body->mass( mass );
    body->linear_damping( 0.1f );
    body->angular_damping( 1.0f );
    body->iit_local( get_block_inertia( glm::vec3( 1.0f ), mass ) );

    dest.mMode = mode;
    dest.mBody.reset( body );
    dest.sync();
}

void application::toggle_culling( void )
{
    drawAll = !drawAll;

    if ( !drawAll )
    {
        camera->mViewParams.mMoveStep = OP_DEFAULT_MOVE_STEP;
    }
    else
    {
        camera->mViewParams.mMoveStep = OP_DEFAULT_MOVE_STEP * 10.0f;
    }
}

void application::draw( void )
{
    const view_data& vp = camera->view_params();

    GL_CHECK( glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ) );

    if ( drawAll )
    {
        draw_group( *this, vp, gen->mBillboards, gen->mWalls, gen->mFreeSpace );
    }
    else
    {
        draw_group( *this, vp, billboards, walls, freeSpace );
    }

    Draw_Axes( *this, vp );
    Draw_Hud( *this );
}

