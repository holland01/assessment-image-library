#include "entity.h"
#include "physics.h"
#include "geom.h"

namespace {

    using entity_dep_table_t =
        std::array< std::function< void( bounds_primitive_t* prim, body_t* body ) >, NUM_BOUNDS_PRIMTYPE >;

    entity_dep_table_t gSyncBodyDepTable =
    {{
        // BOUNDS_PRIMTYPE_HALFSPACE
        []( bounds_primitive_t* prim, body_t* body )
        {
            UNUSEDPARAM( prim );
            UNUSEDPARAM( body );
        },

        // BOUNDS_PRIMTYPE_BOX
        []( bounds_primitive_t* prim, body_t* body )
        {
            bounding_box_t* box = ( bounding_box_t* )prim;

            if ( box )
            {
                box->SetCenter( body->GetPosition() );
                box->SetOrientation( body->GetOrientation() );
            }
        },

        // BOUNDS_PRIMTYPE_LOOKUP
        []( bounds_primitive_t* prim, body_t* body )
        {
            UNUSEDPARAM( prim );
            UNUSEDPARAM( body );
        }
    }};

    entity_dep_table_t gSyncBoundsDepTable =
    {{
        // BOUNDS_PRIMTYPE_HALFSPACE
        []( const bounds_primitive_t* prim, body_t* body )
        {
            UNUSEDPARAM( prim );
            UNUSEDPARAM( body );
        },

        // BOUNDS_PRIMTYPE_BOX
        []( const bounds_primitive_t* prim, body_t* body )
        {
            bounding_box_t* box = ( bounding_box_t* )prim;

            if ( body )
            {
                body->SetFromTransform( box->GetTransform() );
            }
        },

        // BOUNDS_PRIMTYPE_LOOKUP
        []( const bounds_primitive_t* prim, body_t* body )
        {
            UNUSEDPARAM( prim );
            UNUSEDPARAM( body );
        }
    }};
}

entity_bounds_primitive_t::entity_bounds_primitive_t( uint32_t flags, bounds_primitive_t* bounds_ )
    : usageFlags( flags ),
      bounds( bounds_ ),
      next( nullptr )
{
}

//-------------------------------------------------------------------------------------------------------
// entity_bounds_primitive_t
//-------------------------------------------------------------------------------------------------------

entity_t::entity_t( dependent_t dep, body_t* body_, const glm::vec4& color_ )
    : bounds( nullptr ),
      depType( dep ),
      color( color_ ),
      size( 1.0f ),
      body( body_ )
{
}

void entity_t::AddBounds( uint32_t usageFlags, bounds_primitive_t* newBounds )
{
    std::unique_ptr< entity_bounds_primitive_t >* b = &bounds;

    while ( *b )
    {
        if ( usageFlags & ( *b )->usageFlags )
        {
            ( *b )->usageFlags ^= usageFlags;
        }

        b = &( *b )->next;
    }

    ( *b ).reset( new entity_bounds_primitive_t( usageFlags, newBounds ) );
}

namespace {

   INLINE bounds_primitive_t* __QueryBounds( entity_bounds_primitive_t* b, uint32_t flags )
    {
        while ( b )
        {
            if ( b->usageFlags & flags )
            {
                return b->bounds.get();
            }

            b = b->next.get();
        }

        return nullptr;
    }
}

bounds_primitive_t* entity_t::QueryBounds( uint32_t flags )
{
    return __QueryBounds( bounds.get(), flags );
}

const bounds_primitive_t* entity_t::QueryBounds( uint32_t flags ) const
{
    return __QueryBounds( bounds.get(), flags );
}

namespace {
    const  uint32_t NUM_ENTITY_BOUNDS_PRIM_TYPES = 3;

    std::array< uint32_t, NUM_ENTITY_BOUNDS_PRIM_TYPES + 1 > gEntBoundsPrims =
    {
        ENTITY_BOUNDS_ALL,
        ENTITY_BOUNDS_AIR_COLLIDE,
        ENTITY_BOUNDS_AREA_EVAL,
        ENTITY_BOUNDS_MOVE_COLLIDE
    };
}

void entity_t::Sync( void )
{
    for ( auto key: gEntBoundsPrims )
    {
        bounds_primitive_t* prim = QueryBounds( key );

        if ( prim )
        {
            switch ( depType )
            {
                case entity_t::BOUNDS_DEPENDENT:
                    gSyncBoundsDepTable[ prim->type ]( prim, body.get() );
                    break;

                case entity_t::BODY_DEPENDENT:
                    gSyncBodyDepTable[ prim->type ]( prim, body.get() );
                    break;
            }
        }

    }
}
