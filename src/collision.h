#pragma once

#include "def.h"
#include <stdint.h>
#include <memory>
#include <vector>
#include <array>
#include <glm/vec3.hpp>


struct entity_t;
struct bounds_primitive_t;
struct half_space_t;
struct bounding_box_t;

//-------------------------------------------------------------------------------------------------------
// collision_entity_t
//-------------------------------------------------------------------------------------------------------

struct collision_entity_t
{
    std::weak_ptr< entity_t > a;
    std::weak_ptr< entity_t > b;

    bool colliding;
    glm::vec3 normal;

    collision_entity_t( std::weak_ptr< entity_t > a = std::weak_ptr< entity_t >(),
                        std::weak_ptr< entity_t > b = std::weak_ptr< entity_t >() );
};

//-------------------------------------------------------------------------------------------------------
// collision_provider_t
//-------------------------------------------------------------------------------------------------------

enum collision_face_t
{
    COLLISION_FACE_LEFT = 0,
    COLLISION_FACE_FORWARD,
    COLLISION_FACE_RIGHT,
    COLLISION_FACE_BACK,
    NUM_COLLISION_FACES
};

using collision_face_table_t = std::array< int32_t, NUM_COLLISION_FACES >; // indices != -1 are valid faces to collide against; each non-negative index points to an entry within some kind of table

struct collision_provider_t
{
    std::vector< collision_face_table_t >   halfSpaceTable;
    std::vector< half_space_t >             halfSpaces;

    uint32_t GenHalfSpace( const bounding_box_t& bounds, collision_face_t face ); // returns the index of the half space in halfSpaces

    bool BoundsCollidesHalfSpace( collision_entity_t& ce ) const;
};



