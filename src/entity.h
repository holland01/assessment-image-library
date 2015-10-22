#pragma once

#include "geom/geom.h"
#include <array>
#include <queue>
#include <glm/vec4.hpp>
#include <glm/gtc/matrix_transform.hpp>

//-------------------------------------------------------------------------------------------------------
// entity
//-------------------------------------------------------------------------------------------------------

#define ENTITY_INDEX_BITS 22
#define ENTITY_GENERATION_BITS 8
#define ENTITY_INDEX_MASK ( ( 1 << ENTITY_INDEX_BITS ) - 1 )
#define ENTITY_GENERATION_MASK ( ( 1 << ENTITY_GENERATION_BITS ) - 1 )

struct entity
{
friend struct entity_manager;

private:
    uint32_t mId = 0;

public:

    uint32_t index( void ) const { return mId & ENTITY_INDEX_MASK; }

    uint8_t generation( void ) const { return ( mId >> ENTITY_INDEX_BITS ) & ENTITY_GENERATION_MASK; }

};

//-------------------------------------------------------------------------------------------------------
// entity_manager
//
// Idea stolen from
// http://bitsquid.blogspot.com/2014/08/building-data-oriented-entity-system.html
//-------------------------------------------------------------------------------------------------------

struct entity_manager
{
private:
    static const uint32_t MIN_FREE_INDICES = 1024;

    std::vector< uint8_t > mGenerations;

    std::queue< uint32_t > mFreeIndices;

public:

    entity make_entity( void );

    FORCEINLINE bool alive( entity e ) const { return e.generation() == mGenerations[ e.index() ]; }

    FORCEINLINE void destroy( entity e );
};

FORCEINLINE void entity_manager::destroy( entity e )
{
    uint32_t i = e.index();
    mGenerations[ i ]++;
    mFreeIndices.push( i );
}


