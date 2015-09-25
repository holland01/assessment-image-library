#include "collision.h"
#include "geom/geom.h"
#include "entity.h"
#include "debug.h"
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

    contact calc_interpen_depth( const contact& e, const obb& a, const obb& b )
    {
        glm::vec3 normal( ( e.mPoint - a.origin() ) );

        ray r( a.origin(), normal );
        if ( !b.ray_intersection( r, false ) )
		{
            debug_set_flag( true );
            debug_set_ray( r );
		}

		glm::vec3 depth( r.d * r.t );

        return std::move( contact( e.mPoint,
								   e.mNormal,
                                   glm::length( normal - depth ) ) );
    }

    using collision_entity_fn_t = std::function< void( collision_entity& e, const bounds_primitive* collider, const bounds_primitive* collidee ) >;
    using collision_fn_table_t = std::array< collision_entity_fn_t, NUM_BOUNDS_PRIMTYPE >;

    collision_fn_table_t gLookupCollisionTable =
    {{
        // HALFSPACE
        []( collision_entity& e, const bounds_primitive* pa, const bounds_primitive* pb )
        {
            const obb& a = *( pa->to_box() );
            const primitive_lookup& b = *( pb->to_lookup() );

            if ( b.index < 0 )
            {
                e.mColliding = false;
                return;
            }

            const collision_face_table_t& table = e.mProvider.halfSpaceTable[ b.index ];

            for ( int32_t i: table )
            {
                if ( i >= 0 )
                {
                    const halfspace& hs = e.mProvider.halfSpaces[ i ];

                    contact::list_t contacts;

                    if ( a.intersects( contacts, hs ) )
                    {
                        e.mColliding = true;
                        break;
                    }
                }
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
                const bounds_primitive* pa,
                const bounds_primitive* pb )
            {
                assert( pa->type == BOUNDS_PRIM_BOX );
                assert( pb->type == BOUNDS_PRIM_BOX );

                const obb& a = *( pa->to_box() );
                const obb& b = *( pb->to_box() );

                contact::list_t internal, external;

                e.mColliding = a.intersects( internal, b );

                if ( !internal.empty() )
                {
                    external.reserve( internal.size() );

                    for ( const auto& c: internal )
                    {
                        external.insert( calc_interpen_depth( c, a, b ) );
                    }

                    e.mContacts = std::move( external );
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

collision_entity::collision_entity(     const collision_provider& provider_,
                                        ptr_t a_,
                                        ptr_t b_,
                                        const uint32_t colliderBoundsUseFlags,
                                        const uint32_t collideeBoundsUseFlags )
    :
      mEntityAUseFlags( colliderBoundsUseFlags ),
      mEntityBUseFlags( collideeBoundsUseFlags ),
      mEntityA( a_ ),
      mEntityB( b_ ),
      mColliding( false ),
      mProvider( provider_ )
{
}

//-------------------------------------------------------------------------------------------------------
// collision_provider_t
//-------------------------------------------------------------------------------------------------------

uint32_t collision_provider::gen_half_space( const obb& bounds, collision_face face )
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

bool collision_provider::eval_collision( collision_entity& ce ) const
{
    const bounds_primitive* a = ce.mEntityA->query_bounds( ce.mEntityAUseFlags );
    const bounds_primitive* b = ce.mEntityB->query_bounds( ce.mEntityBUseFlags );

    assert( a );
    assert( b );

    gCollisionTable[ a->type ][ b->type ]( ce, a, b );

    return ce.mColliding;
}
