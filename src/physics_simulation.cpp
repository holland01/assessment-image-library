#include "physics_simulation.h"
#include "debug_app.h"
#include "application_update.h"

using namespace std;

namespace {
    vector< entity* > gEnts = []( void ) -> vector< entity* >
    {
        vector< entity* > ents;

        entity* box0 = new entity( entity::BODY_DEPENDENT, new rigid_body(), glm::vec4( 1.0f, 0.0f, 0.0f, 1.0f ) );
        box0->mBody->position( glm::vec3( 0.0f, 10.0f, 0.0f ) );
        box0->mBody->mass( 100.0f );
        box0->mBody->apply_force( glm::vec3( 0.0f, -9.8f, 0.0f ) * box0->mBody->mass() );
        box0->add_bounds( ENTITY_BOUNDS_ALL, new obb() );
        ents.push_back( box0 );

        entity* box1 = new entity( entity::BODY_DEPENDENT, new rigid_body(), glm::vec4( 0.0f, 0.0f, 1.0f, 1.0f ) );
        box1->mBody->position( glm::vec3( 0.0f ) );
        box1->mBody->mass( 8.0f );
        box1->add_bounds( ENTITY_BOUNDS_ALL, new obb() );
        ents.push_back( box1 );

        return std::move( ents );
    }();
}

physics_simulation::physics_simulation( uint32_t w, uint32_t h )
    : physics_app_t( w, h )
{
    camera = &spec;
    camera->mViewParams.mMoveStep = 0.01f;
    camera->position( glm::vec3( 0.0f, 0.0f, 10.0f ) );
}

void physics_simulation::frame( void )
{
    application_frame< physics_simulation > theFrame( *this );
}

void physics_simulation::draw( void )
{
    bind_program program( "single_color" );

    collision_entity ce( collision, gEnts[ 0 ], gEnts[ 1 ] );

    if ( collision.EvalCollision( ce ) )
    {
        gEnts[ 0 ]->mColor = glm::vec4( 0.0f, 1.0f, 0.0f, 1.0f );
        gEnts[ 1 ]->mColor = glm::vec4( 0.0f, 1.0f, 0.0f, 1.0f );

        ce.normal = ce.collider->mBody->position() - ce.collidee->mBody->position();

        float closingVelocity = glm::dot( ce.collidee->mBody->linear_velocity() -
                                          ce.collider->mBody->linear_velocity(),
                                          ce.normal );

        glm::vec3 vNormal( ce.normal * closingVelocity * 0.7f * ce.collidee->mBody->mass() );

        ce.collider->mBody->apply_force_at_point( vNormal, ce.collider->mBody->position() );
    }

    for ( auto e: gEnts )
    {
        const obb& bounds = *ENTITY_PTR_GET_BOX( e, ENTITY_BOUNDS_ALL );
        debug_draw_bounds( *this, bounds, glm::vec3( e->mColor ), e->mColor.a );
    }

    application< physics_simulation >::draw();
}

void physics_simulation::fill_entities( std::vector< entity* >& entities ) const
{
    entities.insert( entities.end(), gEnts.begin(), gEnts.end() );
}
