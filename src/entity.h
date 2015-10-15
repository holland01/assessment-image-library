#pragma once

#include "geom/geom.h"
#include <memory>
#include <glm/vec4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "physics_entity.h"

struct render_pipeline;
struct view_data;

//-------------------------------------------------------------------------------------------------------
// entity
//-------------------------------------------------------------------------------------------------------

struct physics_world;

struct entity
{
protected:
    friend struct map_tile;
    friend struct map_tile_generator;

    std::unique_ptr< physics_entity > mPhysEnt;

public:

    glm::vec4 mColor;

    glm::vec3 mSize;

    entity( const glm::vec4& color = glm::vec4( 1.0f ) );

    void orient_to( const glm::vec3& v );

    glm::mat4 scale_transform( void ) const { return glm::scale( glm::mat4( 1.0f ), mSize ); }

    const physics_entity& physics_data( void ) const { assert( mPhysEnt ); return *mPhysEnt; }

    virtual void sync( void );
};




