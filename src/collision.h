#pragma once

#include "def.h"
#include "entity.h"
#include <stdint.h>
#include <memory>
#include <vector>
#include <array>
#include <glm/vec3.hpp>

struct bounds_primitive;
struct halfspace;
struct obb;

//-------------------------------------------------------------------------------------------------------
// collision_entity_t
//-------------------------------------------------------------------------------------------------------

struct collision_provider;

struct collision_entity
{
	using ptr_t = entity*;

    const uint32_t colliderUseFlags;
    const uint32_t collideeUseFlags;

	ptr_t collider;
	ptr_t collidee;

    bool colliding = false;
    glm::vec3 normal = glm::vec3( 0.0f );

    float interpenDepth = 0.0f;

    const collision_provider& provider;

    collision_entity( const collision_provider& provider,
						ptr_t collider = nullptr,
						ptr_t collidee = nullptr,
                        const uint32_t colliderBoundsUseFlags = ENTITY_BOUNDS_ALL,
                        const uint32_t collideeBoundsUseFlags = ENTITY_BOUNDS_ALL );
};

//-------------------------------------------------------------------------------------------------------
// collision_provider_t
//-------------------------------------------------------------------------------------------------------

enum collision_face
{
    COLLISION_FACE_LEFT = 0,
    COLLISION_FACE_FORWARD,
    COLLISION_FACE_RIGHT,
    COLLISION_FACE_BACK,
    NUM_COLLISION_FACES
};

using collision_face_table_t = std::array< int32_t, NUM_COLLISION_FACES >; // indices != -1 are valid faces to collide against; each non-negative index points to an entry within some kind of table

struct collision_provider
{
    std::vector< collision_face_table_t >   halfSpaceTable;
    std::vector< halfspace >             halfSpaces;

    uint32_t GenHalfSpace( const obb& bounds, collision_face face ); // returns the index of the half space in halfSpaces

    bool EvalCollision( collision_entity& ce ) const;
};



