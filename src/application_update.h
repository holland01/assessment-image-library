#ifndef APPLICATION_UPDATE_INL
#define APPLICATION_UPDATE_INL

#include "physics_world_update.inl"

template < typename child_t >
struct application_frame
{
    using app_t = application< child_t >;

    app_t& app;

    application_frame( app_t& a );

    ~application_frame( void );
};

template< typename child_t >
application_frame< child_t >::application_frame( app_t& a )
    : app( a )
{
    app.startTime = get_time();

    const view_data& vp = app.camera->view_params();

    app.frustum.update( vp );
}

template< typename child_t >
application_frame< child_t >::~application_frame( void )
{
    app.world.update( app );

    app.draw();

    // clear bodies which are added in world.Update call,
    // since we only want to integrate bodies which are in view
    app.world.mTime = get_time() - app.startTime;

    printf( "FPS: %f\r", 1.0f / app.world.mTime );
}

#endif // APPLICATION_UPDATE_INL

