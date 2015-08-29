#include "collision.h"
#include "geom.h"
#include "entity.h"
#include <glm/glm.hpp>

#define DUMMY_LAMBDA \
    []( collision_entity& e, const bounds_primitive* collider, const bounds_primitive* collidee ) \
    { \
         assert( false ); \
         UNUSEDPARAM( e ); \
         UNUSEDPARAM( collider ); \
         UNUSEDPARAM( collidee ); \
    }

namespace {

    void calc_interpen_depth( collision_entity& e, const obb& a, const obb& b )
    {
        glm::vec3 normal( b.center() - a.center() );

        ray r( a.center(), normal );

        float t = 0.0f;
        b.ray_intersection( t, r, false );

        assert( t != FLT_MAX );

        glm::vec3 depth( r.d * t );

        e.interpenDepth = glm::length( normal - depth );
    }

    using collision_entity_fn_t = std::function< void( collision_entity& e, const bounds_primitive* collider, const bounds_primitive* collidee ) >;
    using collision_fn_table_t = std::array< collision_entity_fn_t, NUM_BOUNDS_PRIMTYPE >;

    collision_fn_table_t gLookupCollisionTable =
    {{
        // HALFSPACE
        []( collision_entity& e, const bounds_primitive* collider, const bounds_primitive* collidee )
        {
            const obb& a = *( collider->to_box() );
            const primitive_lookup& b = *( collidee->to_lookup() );

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
                    const halfspace& hs = e.provider.halfSpaces[ i ];

                    if ( a.intersects( e.normal, hs ) )
                    {
                        if ( glm::length( e.normal ) < 1.0f )
                        {
                            e.normal = glm::normalize( e.normal );
                        }

                        e.colliding = true;
                        break;
                    }
                }
            }

            if ( e.colliding )
            {
                const obb& bounds = *ENTITY_PTR_GET_BOX( e.collidee, ENTITY_BOUNDS_AREA_EVAL );

                calc_interpen_depth( e, a, bounds );

            }
        },
        DUMMY_LAMBDA,
        DUMMY_LAMBDA
     }};

    collision_entity_fn_t gDoLookupFn = []( collision_entity& e,
        const bounds_primitive* cer, const bounds_primitive* cee )
    {
        assert( cee->type == BOUNDS_PRIM_LOOKUP );
        gLookupCollisionTable[ cee->to_lookup()->lookupType ]( e, cer, cee );
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
            []( collision_entity& e,
                const bounds_primitive* collider,
                const bounds_primitive* collidee )
            {
                assert( collider->type == BOUNDS_PRIM_BOX );
                assert( collidee->type == BOUNDS_PRIM_BOX );

                const obb& a = *( collider->to_box() );
                const obb& b = *( collidee->to_box() );

                e.colliding = a.intersects( e.normal, b );

                if ( e.colliding )
                {
                    calc_interpen_depth( e, a, b );
                }
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

collision_entity::collision_entity( const collision_provider& provider_,
                                        const entity* a_,
                                        const entity* b_,
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

uint32_t collision_provider::GenHalfSpace( const obb& bounds, collision_face face )
{
    std::array< glm::vec3, NUM_COLLISION_FACES > halfSpaceNormals =
    {{
        glm::vec3( -1.0f, 0.0f, 0.0f ),
        glm::vec3( 0.0f, 0.0f, -1.0f ),
        glm::vec3( 1.0f, 0.0f, 0.0f ),
        glm::vec3( 0.0f, 0.0f, 1.0f )
    }};

    uint32_t index = ( int32_t ) halfSpaces.size();

    halfspace hs( bounds, halfSpaceNormals[ face ] );
    halfSpaces.push_back( std::move( hs ) );

    return index;
}

bool collision_provider::EvalCollision( collision_entity& ce ) const
{
    const bounds_primitive* collider = ce.collider->query_bounds( ce.colliderUseFlags );
    const bounds_primitive* collidee = ce.collidee->query_bounds( ce.collideeUseFlags );

    assert( collider );
    assert( collidee );

    gCollisionTable[ collider->type ][ collidee->type ]( ce, collider, collidee );

    return ce.colliding;
}
