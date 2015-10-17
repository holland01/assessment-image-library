#pragma once

#include "def.h"

#include <bullet3/btBulletCollisionCommon.h>
#include "glm_ext.hpp"

// Utility functions for bullet API

namespace btext {

INLINE void translate_motion_state( btMotionState& ms, const glm::vec3& v )
{
    btVector3 position( glm::ext::to_bullet( v ) );

    btTransform t;
    ms.getWorldTransform( t );
    t.setOrigin( position );
    ms.setWorldTransform( t );
}

}
