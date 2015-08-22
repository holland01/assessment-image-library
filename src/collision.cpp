#include "collision.h"
#include "geom.h"
#include "entity.h"
#include <glm/glm.hpp>

#define DUMMY_LAMBDA \
    []( collision_entity_t& e, const bounds_primitive_t* collider, const bounds_primitive_t* collidee ) \
    { \
         assert( false ); \
         UNUSEDPARAM( e ); \
         UNUSEDPARAM( collider ); \
         UNUSEDPARAM( collidee ); \
    }

namespace {

    using collision_entity_fn_t = std::function< void( collision_entity_t& e, const bounds_primitive_t* collider, const bounds_primitive_t* collidee ) >;
    using collision_fn_table_t = std::array< collision_entity_fn_t, NUM_BOUNDS_PRIMTYPE >;

    collision_fn_table_t gLookupCollisionTable =
    {{
        // HALFSPACE
        []( collision_entity_t& e, const bounds_primitive_t* collider, const bounds_primitive_t* collidee )
        {
            const bounding_box_t& a = *( collider->ToBox() );
            const primitive_lookup_t& b = *( collidee->ToLookup() );

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
        DUMMY_LAMBDA,
        DUMMY_LAMBDA
     }};

    collision_entity_fn_t gDoLookupFn = []( collision_entity_t& e,
        const bounds_primitive_t* cer, const bounds_primitive_t* cee )
    {
        assert( cee->type == BOUNDS_PRIM_LOOKUP );
        gLookupCollisionTable[ cee->ToLookup()->lookupType ]( e, cer, cee );
    };

    std::array< collision_fn_table_t, NUM_BOUNDS_PRIMTYPE > gCollisionTable =
    {{
        // HALFSPACE
        {{
            DUMMY_LAMBDA,
            DUMMY_LAMBDA,
            gDoLookupFn,
        }},

        // BOX
        {{
            // BOUNDS_HALFSPACE
            DUMMY_LAMBDA,

            // BOUNDS_BOUNDS
            []( collision_entity_t& e,
                const bounds_primitive_t* collider,
                const bounds_primitive_t* collidee )
            {
                assert( collider->type == BOUNDS_PRIM_BOX );
                assert( collidee->type == BOUNDS_PRIM_BOX );

                const bounding_box_t& a = *( collider->ToBox() );
                const bounding_box_t& b = *( collidee->ToBox() );

                e.colliding = a.IntersectsBounds( e.normal, b );
            },
            gDoLookupFn,
        }},

        // LOOKUP
        {{
            DUMMY_LAMBDA,
            DUMMY_LAMBDA,
            DUMMY_LAMBDA
        }}
    }};
}

//-------------------------------------------------------------------------------------------------------
// collision_entity_t
//-------------------------------------------------------------------------------------------------------

collision_entity_t::collision_entity_t( const collision_provider_t& provider_,
                                        const entity_t* a_,
                                        const entity_t* b_,
                                        const uint32_t colliderBoundsUseFlags,
                                        const uint32_t collideeBoundsUseFlags )
    :
      colliderUseFlags( colliderBoundsUseFlags ),
      collideeUseFlags( collideeBoundsUseFlags ),
      collider( a_ ),
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
    const bounds_primitive_t* collider = ce.collider->QueryBounds( ce.colliderUseFlags );
    const bounds_primitive_t* collidee = ce.collidee->QueryBounds( ce.collideeUseFlags );

    assert( collider );
    assert( collidee );

    gCollisionTable[ collider->type ][ collidee->type ]( ce, collider, collidee );

    return ce.colliding;
}
