#include "collision.h"
#include "geom.h"
#include "entity.h"
#include <glm/glm.hpp>

namespace {

    using collision_entity_fn_t = std::function< void( collision_entity_t& e ) >;
    using collision_fn_table_t = std::array< collision_entity_fn_t, NUM_BOUNDS_PRIMTYPE >;

    collision_fn_table_t gLookupCollisionTable =
    {{
        // HALFSPACE
        []( collision_entity_t& e )
        {
            const bounding_box_t& a = *( e.collider->GetBoundsAsBox() );
            const primitive_lookup_t& b = *( e.collidee->GetBoundsAsLookup() );

            if ( b.index < 0 )
            {
                e.colliding = false;
                return;
            }

            const collision_face_table_t& table = e.provider.halfSpaceTable[ b.index ];

            for ( int32_t i: table )
            {
                if ( i >= 0 )
                {
                    const half_space_t& hs = e.provider.halfSpaces[ i ];

                    if ( a.IntersectsHalfSpace( e.normal, hs ) )
                    {
                        if ( glm::length( e.normal ) < 1.0f )
                        {
                            e.normal = glm::normalize( e.normal );
                        }

                        e.colliding = true;
                        return;
                    }
                }
            }
        },
        []( collision_entity_t& e ) { assert( false ); UNUSEDPARAM( e ); },
        []( collision_entity_t& e ) { assert( false ); UNUSEDPARAM( e ); },
    }};

    collision_entity_fn_t gDoLookupFn = []( collision_entity_t& e )
    {
        assert( e.collidee->collisionBounds->type == BOUNDS_PRIM_LOOKUP );
        gLookupCollisionTable[ e.collidee->GetBoundsAsLookup()->lookupType ]( e );
    };

    std::array< collision_fn_table_t, NUM_BOUNDS_PRIMTYPE > gCollisionTable =
    {{
        // HALFSPACE
        {{
            []( collision_entity_t& e ) { assert( false ); UNUSEDPARAM( e ); },
            []( collision_entity_t& e ) { assert( false ); UNUSEDPARAM( e ); },
            gDoLookupFn,
        }},

        // BOX
        {{
            // BOUNDS_HALFSPACE
            []( collision_entity_t& e ) { assert( false ); UNUSEDPARAM( e ); },

            // BOUNDS_BOUNDS
            []( collision_entity_t& e )
            {
                assert( e.collider->collisionBounds->type == BOUNDS_PRIM_BOX );
                assert( e.collidee->collisionBounds->type == BOUNDS_PRIM_BOX );

                const bounding_box_t& a = *( e.collider->GetBoundsAsBox() );
                const bounding_box_t& b = *( e.collidee->GetBoundsAsBox() );

                e.colliding = a.IntersectsBounds( e.normal, b );
            },
            gDoLookupFn,
        }},

        // LOOKUP
        {{
            []( collision_entity_t& e ) { assert( false ); UNUSEDPARAM( e ); },
            []( collision_entity_t& e ) { assert( false ); UNUSEDPARAM( e ); },
            []( collision_entity_t& e ) { assert( false ); UNUSEDPARAM( e ); }
        }}
    }};
}

//-------------------------------------------------------------------------------------------------------
// collision_entity_t
//-------------------------------------------------------------------------------------------------------

collision_entity_t::collision_entity_t( const collision_provider_t& provider_,
                                        const entity_t* a_,
                                        const entity_t* b_ )
    : collider( a_ ),
      collidee( b_ ),
      colliding( false ),
      normal( 0.0f ),
      provider( provider_ )
{
}

//-------------------------------------------------------------------------------------------------------
// collision_provider_t
//-------------------------------------------------------------------------------------------------------

uint32_t collision_provider_t::GenHalfSpace( const bounding_box_t& bounds, collision_face_t face )
{
    std::array< glm::vec3, NUM_COLLISION_FACES > halfSpaceNormals =
    {{
        glm::vec3( -1.0f, 0.0f, 0.0f ),
        glm::vec3( 0.0f, 0.0f, -1.0f ),
        glm::vec3( 1.0f, 0.0f, 0.0f ),
        glm::vec3( 0.0f, 0.0f, 1.0f )
    }};

    uint32_t index = ( int32_t ) halfSpaces.size();

    half_space_t hs( bounds, halfSpaceNormals[ face ] );
    halfSpaces.push_back( std::move( hs ) );

    return index;
}

bool collision_provider_t::EvalCollision( collision_entity_t& ce ) const
{
    assert( ce.collider->collisionBounds );
    assert( ce.collidee->collisionBounds );

    gCollisionTable[ ce.collider->collisionBounds->type ][ ce.collidee->collisionBounds->type ]( ce );

    return ce.colliding;
}
