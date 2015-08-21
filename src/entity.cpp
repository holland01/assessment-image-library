#include "entity.h"
#include "physics.h"
#include "geom.h"

namespace {
    std::array<
        std::function< void( bounds_primitive_t* prim, const body_t* body ) >,
        NUM_BOUNDS_PRIMTYPE >

    gSyncBodyDepTable =
    {{
        // BOUNDS_PRIMTYPE_HALFSPACE
        []( bounds_primitive_t* prim, const body_t* body )
        {
            UNUSEDPARAM( prim );
            UNUSEDPARAM( body );
        },

        // BOUNDS_PRIMTYPE_BOX
        []( bounds_primitive_t* prim, const body_t* body )
        {
            bounding_box_t* box = ( bounding_box_t* )prim;

            if ( box )
            {
                box->SetCenter( body->GetPosition() );
                box->SetOrientation( body->GetOrientation() );
            }
        },

        // BOUNDS_PRIMTYPE_LOOKUP
        []( bounds_primitive_t* prim, const body_t* body )
        {
            UNUSEDPARAM( prim );
            UNUSEDPARAM( body );
        }
    }};

    std::array<
        std::function< void( const bounds_primitive_t* prim, body_t* body ) >,
        NUM_BOUNDS_PRIMTYPE >

    gSyncBoundsDepTable =
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

entity_t::entity_t( dependent_t dep, bounds_primitive_t* bounds_, body_t* body_, const glm::vec4& color_ )
    : depType( dep ),
      color( color_ ),
      size( 1.0f ),
      bounds( bounds_ ),
      body( body_ )
{
}

void entity_t::Sync( void )
{
    switch ( depType )
    {
        case entity_t::BOUNDS_DEPENDENT:
            if ( bounds )
            {
                gSyncBoundsDepTable[ bounds->type ]( bounds.get(), body.get() );
            }
            break;
        case entity_t::BODY_DEPENDENT:
            if ( bounds )
            {
                gSyncBodyDepTable[ bounds->type ]( bounds.get(), body.get() );
            }
            break;
    }
}
