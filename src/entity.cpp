#include "entity.h"
#include "geom/geom.h"
#include "physics_world.h"

//-------------------------------------------------------------------------------------------------------
// entity_bounds_primitive_t
//-------------------------------------------------------------------------------------------------------

entity::entity(const glm::vec4& color )
    : mPhysEnt( nullptr ),
      mColor( color ),
      mSize( 1.0f )
{
}

void entity::orient_to( const glm::vec3& v )
{
    if ( !mPhysEnt )
        return;

    btDefaultMotionState& motionState = mPhysEnt->motion_state();

    btTransform worldTrans;
    motionState.getWorldTransform( worldTrans );

    glm::vec3 boundsOrigin( glm::ext::from_bullet( worldTrans.getOrigin() ) );

    glm::vec3 dir( v - boundsOrigin );
    dir.y = 0.0f;
    dir = glm::normalize( dir );

    glm::mat3 orient(
        orient_by_direction(
                dir,
                glm::vec3( 0.0f, 0.0f, 1.0f ),
                glm::vec3( -1.0f, 0.0f, 0.0f )
        )
    );

    glm::quat q( glm::quat_cast( orient ) );
    worldTrans.setRotation( glm::ext::to_bullet( q ) );
    motionState.setWorldTransform( worldTrans );
}

void entity::sync( void )
{
    if ( !mPhysEnt || !mPhysEnt->mMotionState )
        return;
}
