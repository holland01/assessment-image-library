#pragma once

#include "def.h"
#include "entity.h"
#include "collision_contact.h"
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

    const uint32_t mEntityAUseFlags;
    const uint32_t mEntityBUseFlags;

    ptr_t mEntityA;
    ptr_t mEntityB;

    std::vector< contact > mContacts;

    bool mColliding = false;

    const collision_provider& mProvider;

    collision_entity(   const collision_provider& provider,
                        ptr_t entA = nullptr,
                        ptr_t entB = nullptr,
                        const uint32_t entityAUseFlags = ENTITY_BOUNDS_ALL,
                        const uint32_t entityBUseFlags = ENTITY_BOUNDS_ALL );
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

    uint32_t gen_half_space( const obb& bounds, collision_face face ); // returns the index of the half space in halfSpaces

    bool eval_collision( collision_entity& ce ) const;
};



