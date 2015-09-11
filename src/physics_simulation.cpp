#include "physics_simulation.h"
#include "debug_app.h"
#include "application_update.h"

using namespace std;

namespace {
    vector< entity* > gEnts = []( void ) -> vector< entity* >
    {
        vector< entity* > ents;

        uint32_t flags = rigid_body::RESET_FORCE_ACCUM | rigid_body::RESET_TORQUE_ACCUM;

        entity* box0 = new entity( entity::BODY_DEPENDENT, new rigid_body( flags ), glm::vec4( 1.0f, 0.0f, 0.0f, 1.0f ) );
        box0->mBody->position( glm::vec3( 0.0f, 10.0f, 0.0f ) );
        box0->mBody->mass( 100.0f );
        box0->mBody->linear_damping( 0.3f );
        box0->mBody->angular_damping( 0.5f );
        box0->add_bounds( ENTITY_BOUNDS_ALL, new obb() );
        box0->mBody->iit_local( get_block_inertia( glm::vec3( 1.0f ), box0->mBody->mass() ) );

        ents.push_back( box0 );

        entity* box1 = new entity( entity::BODY_DEPENDENT, new rigid_body( flags ), glm::vec4( 0.0f, 0.0f, 1.0f, 1.0f ) );
        box1->mBody->position( glm::vec3( 0.0f ) );
        box1->mBody->mass( 110.0f );
        box1->add_bounds( ENTITY_BOUNDS_ALL, new obb() );
        ents.push_back( box1 );

        return std::move( ents );
    }();

    struct force
    {
    protected:
        glm::vec3 mForce;

        vector< collision_entity* > mEntityList;

    public:
        force( void )
            : mForce( 0.0f )
        {}

        glm::vec3 operator()( void ) const { return mForce; }
    };

    struct test_force : public force
    {
    public:
        test_force( const collision_entity& ce )
        {
            float closingVelocity = glm::dot( ce.collidee->mBody->linear_velocity() -
                                              ce.collider->mBody->linear_velocity(),
                                              ce.normal );

            float massDiff = ce.collidee->mBody->mass() - ce.collider->mBody->mass();

            glm::vec3 f( ce.normal * massDiff );

            glm::vec3 v( ce.collider->mBody->linear_velocity() * -closingVelocity );

            glm::vec3 fv( f + v );

            mForce = std::move( fv * 0.1f );
        }
    };

    struct spring_force : public force
    {
    public:
        spring_force( collision_entity& ce, float restLength, float springConstant )
        {
            glm::vec3 theForce( ce.collider->mBody->position() - ce.collidee->mBody->position() );

            float mag = glm::length( theForce );
            mag = glm::abs( mag - restLength );
            mag *= springConstant;

            mForce = glm::normalize( theForce ) * -mag;
        }
    };

    struct gravity_plane
    {
        plane mPlane;
        float mPull;

        gravity_plane( void )
            : mPull( -9.8f )
        {
        }
    };
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

static bool gDoIt = true;

void physics_simulation::draw( void )
{
    bind_program program( "single_color" );

    collision_entity ce( collision, gEnts[ 0 ], gEnts[ 1 ] );

    //spring_force sf( ce, 9.0f, 1.0f );

    if ( collision.EvalCollision( ce ) )
    {
        //gDoIt = false;

        gEnts[ 0 ]->mColor = glm::vec4( 0.0f, 1.0f, 0.0f, 1.0f );
        gEnts[ 1 ]->mColor = glm::vec4( 0.0f, 1.0f, 0.0f, 1.0f );

        ce.normal = ce.collider->mBody->position() - ce.collidee->mBody->position();

        test_force tf( ce );

        gEnts[ 0 ]->mBody->apply_force( tf() );
    }
    else if ( gDoIt )
    {
        gEnts[ 0 ]->mBody->apply_force_at_local_point( glm::vec3( 0.0f, -0.5f, 0.0f ),
                                                       glm::vec3( 0.0f, 0.0f, 0.0f ) );
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
