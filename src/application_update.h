#ifndef APPLICATION_UPDATE_INL
#define APPLICATION_UPDATE_INL

#include "physics_world_update.inl"
#include <glm/gtx/string_cast.hpp>

template < typename child_t >
struct application_frame
{
    using app_t = application< child_t >;

	app_t& mApp;

    application_frame( app_t& a );

    ~application_frame( void );
};

template< typename child_t >
application_frame< child_t >::application_frame( app_t& a )
	: mApp( a )
{
	mApp.startTime = get_time();

    /*
	const view_data& vp = mApp.camera->view_params();

	mApp.frustum.update( vp );

    mApp.world.update( mApp );
*/
	mApp.draw();
}

template< typename child_t >
application_frame< child_t >::~application_frame( void )
{
    // clear bodies which are added in world.Update call,
    // since we only want to integrate bodies which are in view
	mApp.world.mTime = get_time() - mApp.startTime;

    printf( "Position: %s, FPS: %f\r", glm::to_string( mApp.camera->position() ).c_str(),
			1.0f / mApp.world.mTime );
}

#endif // APPLICATION_UPDATE_INL

