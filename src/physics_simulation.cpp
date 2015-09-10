#include "physics_simulation.h"
#include "debug_app.h"
#include "application_update.h"

physics_simulation::physics_simulation( uint32_t w, uint32_t h )
    : physics_app_t( w, h )
{
    camera->mViewParams.mMoveStep = 0.01f;
}

void physics_simulation::frame( void )
{
    application_frame< physics_simulation > theFrame( *this );
    UNUSEDPARAM( theFrame );
}

void physics_simulation::fill_entities( std::vector< entity* >& entities ) const
{
    UNUSEDPARAM( entities );
}
