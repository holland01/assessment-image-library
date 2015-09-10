#ifndef PHYSICS_WORLD_UPDATE_INL
#define PHYSICS_WORLD_UPDATE_INL

#include "application.h"
#include "physics.h"

template< typename child_t >
INLINE void physics_world::update( application< child_t >& app )
{
    mBodies.clear();

    if ( app.camera && app.camera->mBody )
    {
        mBodies.push_back( app.camera );
    }

    app.fill_entities( mBodies );

    app.camera->apply_movement();

    sync_bodies();

    clear_accum();
}

#endif // PHYSICS_WORLD_UPDATE_INL

