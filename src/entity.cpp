#include "entity.h"
#include "physics.h"
#include "geom.h"

namespace {

    using entity_dep_table_t =
        std::array< std::function< void( bounds_primitive* prim, rigid_body* body ) >, NUM_BOUNDS_PRIMTYPE >;

    entity_dep_table_t gSyncBodyDepTable =
    {{
        // BOUNDS_PRIMTYPE_HALFSPACE
        []( bounds_primitive* prim, rigid_body* body )
        {
            UNUSEDPARAM( prim );
            UNUSEDPARAM( body );
        },

        // BOUNDS_PRIMTYPE_BOX
        []( bounds_primitive* prim, rigid_body* body )
        {
            obb* box = ( obb* )prim;

            if ( box )
            {
                box->center( body->position() );
                box->orientation( body->orientation() );
            }
        },

        // BOUNDS_PRIMTYPE_LOOKUP
        []( bounds_primitive* prim, rigid_body* body )
        {
            UNUSEDPARAM( prim );
            UNUSEDPARAM( body );
        }
    }};

    entity_dep_table_t gSyncBoundsDepTable =
    {{
        // BOUNDS_PRIMTYPE_HALFSPACE
        []( const bounds_primitive* prim, rigid_body* body )
        {
            UNUSEDPARAM( prim );
            UNUSEDPARAM( body );
        },

        // BOUNDS_PRIMTYPE_BOX
        []( const bounds_primitive* prim, rigid_body* body )
        {
            obb* box = ( obb* )prim;

            if ( body )
            {
                body->set( box->axes() );
            }
        },

        // BOUNDS_PRIMTYPE_LOOKUP
        []( const bounds_primitive* prim, rigid_body* body )
        {
            UNUSEDPARAM( prim );
            UNUSEDPARAM( body );
        }
    }};
}

entity_bounds_primitive::entity_bounds_primitive( uint32_t flags, bounds_primitive* bounds_ )
    : usageFlags( flags ),
      bounds( bounds_ ),
      next( nullptr )
{
}

//-------------------------------------------------------------------------------------------------------
// entity_bounds_primitive_t
//-------------------------------------------------------------------------------------------------------

entity::entity( dependent_t dep, rigid_body* body_, const glm::vec4& color_ )
    : mBounds( nullptr ),
      mDepType( dep ),
      mColor( color_ ),
      mSize( 1.0f ),
      mBody( body_ )
{
}

void entity::add_bounds( uint32_t usageFlags, bounds_primitive* newBounds )
{
    std::unique_ptr< entity_bounds_primitive >* b = &mBounds;

    while ( *b )
    {
        if ( usageFlags & ( *b )->usageFlags )
        {
            ( *b )->usageFlags ^= usageFlags;
        }

        b = &( *b )->next;
    }

    ( *b ).reset( new entity_bounds_primitive( usageFlags, newBounds ) );
}

namespace {

   INLINE bounds_primitive* __QueryBounds( entity_bounds_primitive* b, uint32_t flags )
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

bounds_primitive* entity::query_bounds( uint32_t flags )
{
    return __QueryBounds( mBounds.get(), flags );
}

const bounds_primitive* entity::query_bounds( uint32_t flags ) const
{
    return __QueryBounds( mBounds.get(), flags );
}

namespace {
    const  uint32_t NUM_ENTITY_BOUNDS_PRIM_TYPES = 3;

    std::array< uint32_t, NUM_ENTITY_BOUNDS_PRIM_TYPES + 1 > gEntBoundsPrims =
    {{
        ENTITY_BOUNDS_ALL,
        ENTITY_BOUNDS_AIR_COLLIDE,
        ENTITY_BOUNDS_AREA_EVAL,
        ENTITY_BOUNDS_MOVE_COLLIDE
    }};
}

void entity::sync( void )
{
    for ( auto key: gEntBoundsPrims )
    {
        bounds_primitive* prim = query_bounds( key );

        if ( prim )
        {
            switch ( mDepType )
            {
                case entity::BOUNDS_DEPENDENT:
                    gSyncBoundsDepTable[ prim->type ]( prim, mBody.get() );
                    break;

                case entity::BODY_DEPENDENT:
                    gSyncBodyDepTable[ prim->type ]( prim, mBody.get() );
                    break;
            }
        }

    }
}
