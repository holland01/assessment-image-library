#include "physics_simulation.h"
#include "debug_app.h"
#include "application_update.h"

using namespace std;

#define GROUND_MASS 10000.0f

namespace {
    const glm::vec4 G_TAG_COLOR( 0.0f, 0.0f, 1.0f, 1.0f );

    vector< entity* > gen_entities( void )
    {
        vector< entity* > ents;

        uint32_t flags = rigid_body::RESET_FORCE_ACCUM | rigid_body::RESET_TORQUE_ACCUM;

        entity* box0 = new entity( entity::BODY_DEPENDENT, new rigid_body( flags ),
                                   glm::vec4( 1.0f, 0.0f, 0.0f, 1.0f ) );
        box0->mBody->position( glm::vec3( -10.0f, 500.0f, 0.0f ) );
        box0->mBody->mass( 100.0f );
        box0->mBody->linear_damping( 0.01f );
        box0->mBody->angular_damping( 0.01f );
        box0->add_bounds( ENTITY_BOUNDS_ALL, new obb() );
        box0->mBody->iit_local( get_block_inertia( glm::vec3( 1.0f ), box0->mBody->mass() ) );

        ents.push_back( box0 );

        entity* box1 = new entity( entity::BODY_DEPENDENT,
                                   new rigid_body( flags ),
                                   G_TAG_COLOR );
        box1->mBody->position( glm::vec3( 0.0f, 100.0f, 0.0f ) );
        box1->mBody->mass( 200.0f );
        box1->mBody->linear_damping( 0.3f );
        box1->mBody->angular_damping( 0.5f );
        box1->add_bounds( ENTITY_BOUNDS_ALL, new obb() );
        box1->mBody->iit_local( get_block_inertia( glm::vec3( 1.0f ), box1->mBody->mass() ) );

        ents.push_back( box1 );

        entity* groundPlane = new entity( entity::BODY_DEPENDENT, new rigid_body( flags ),
                                          glm::vec4( 0.0f, 0.4f, 0.0f, 1.0f ) );
        groundPlane->mBody->position( glm::vec3( 0.0f ) );
        groundPlane->mBody->mass( INFINITE_MASS );
        groundPlane->add_bounds( ENTITY_BOUNDS_ALL,
                                 new obb() );

        groundPlane->mSize = glm::vec3( 100.0f, 1.0f, 100.0f );
        groundPlane->sync_options( ENTITY_SYNC_APPLY_SCALE );

        ents.push_back( groundPlane );

        return std::move( ents );
    }

    vector< entity* > gEnts = gen_entities();

    struct force
    {
    protected:
        glm::vec3 mForce, mFriction;

        std::vector< collision_entity* > mEntityList;

    public:
        force( void )
            : mForce( 0.0f ),
              mFriction( 0.0f )
        {}

        glm::vec3 force_vec( void ) const { return mForce; }

        glm::vec3 friction( void ) const { return mFriction; }
    };

    INLINE float calc_mass_comp( const collision_entity::ptr_t entity )
    {
        if ( entity->mBody->mass() != INFINITE_MASS )
        {
            return entity->mBody->inv_mass();
        }

        return 0.0f;
    }

    struct test_force : public force
    {
    public:
        test_force( const physics_world& world, const contact& c, const collision_entity& ce )
        {
            glm::vec3 resolveDir( glm::normalize( c.mNormal ) );

            glm::vec3 relavel( ce.mEntityB->mBody->linear_velocity() - ce.mEntityA->mBody->linear_velocity() );
            relavel -= c.mNormal * c.mInterpenDepth * world.mTargetDeltaTime;

            float closingVelocity = glm::dot( relavel, resolveDir );

            if ( closingVelocity > 0.0f )
            {
                return;
            }

            const float restitution = 1.0f;

            float impulseNum = -( 1.0f + restitution ) * closingVelocity;
            float denom = ce.mEntityA->mBody->inv_mass() + ce.mEntityB->mBody->inv_mass();

            glm::vec3 relaPointA( c.mPoint - ce.mEntityA->mBody->position() );
            glm::vec3 rra( glm::cross( relaPointA, resolveDir ) );
            glm::vec3 rrra( glm::cross( ce.mEntityA->mBody->iit_world() * rra, relaPointA ) );

            glm::vec3 relaPointB( c.mPoint - ce.mEntityB->mBody->position() );
            glm::vec3 rrb( glm::cross( relaPointB, resolveDir ) );
            glm::vec3 rrrb( glm::cross( ce.mEntityB->mBody->iit_world() * rrb, relaPointB ) );

            float t0 = glm::dot( rrrb + rrra, resolveDir );

            denom += t0;

            float impulse = impulseNum / denom;

            glm::vec3 imp( impulse * resolveDir );
            glm::vec3 impb( ce.mEntityB->mBody->inv_mass() * imp );

            if ( !ce.mEntityA->mBody->is_static() )
            {
                glm::vec3 impa( ce.mEntityA->mBody->inv_mass() * imp );
                ce.mEntityA->mBody->linear_velocity() -= impa;

                glm::vec3 angMom( ce.mEntityA->mBody->angular_velocity() * ce.mEntityA->mBody->mass() );
                angMom += glm::normalize( glm::cross( relaPointA, imp ) );

                ce.mEntityA->mBody->angular_velocity() = ce.mEntityA->mBody->iit_world() * angMom;
            }
            ce.mEntityB->mBody->linear_velocity() += impb;
        }
    };

    struct spring_force : public force
    {
    public:
        spring_force( collision_entity& ce, float restLength, float springConstant )
        {
            glm::vec3 theForce( ce.mEntityB->mBody->position() - ce.mEntityA->mBody->position() );

            float mag = glm::length( theForce );
            mag = glm::abs( mag - restLength );
            mag *= springConstant;

            mForce = glm::normalize( theForce ) * -mag;
        }
    };
}

physics_simulation::physics_simulation( uint32_t w, uint32_t h )
    : physics_app_t( w, h )
{
    camera = &spec;
    camera->mViewParams.mMoveStep = 0.1f;
    camera->position( glm::vec3( 0.0f, 10.0f, 100.0f ) );
}

void physics_simulation::frame( void )
{
    application_frame< physics_simulation > theFrame( *this );
}

namespace {
    bool gDoIt = true;
    bool gFire = false;
}
void physics_simulation::draw( void )
{
    bind_program program( "single_color" );

    auto emit_pred = []( entity* e ) -> bool
    {
        if ( !gFire )
        {
            return e->mColor != G_TAG_COLOR;
        }
        else
        {
            return true;
        }
    };

    auto apply_gravity = [ emit_pred ]( entity* e )
    {
        if ( gDoIt && emit_pred( e ) )
        {
            e->mBody->apply_force( glm::vec3( 0.0f, -9.8f, 0.0f ) );
        }
    };

    for ( entity* e: gEnts )
    {
        for ( entity* e1: gEnts )
        {
            if ( e == e1 )
            {
                continue;
            }

            collision_entity ce( collision, e, e1 );

            if ( collision.eval_collision( ce ) )
            {
                for ( const contact& c: ce.mContacts )
                {
                    //glm::vec3 normal( glm::normalize( ce.mEntityA->mBody->position() - ce.mEntityB->mBody->position() ) );
                    test_force tf( world, c, ce );
                }
            }

            apply_gravity( e );
            apply_gravity( e1 );
        }

        const obb& bounds = *ENTITY_PTR_GET_BOX( e, ENTITY_BOUNDS_ALL );
        debug_draw_bounds( *this, bounds, glm::vec3( e->mColor ), e->mColor.a );
    }

    application< physics_simulation >::draw();
}

void physics_simulation::fill_entities( std::vector< entity* >& entities ) const
{
    entities.insert( entities.end(), gEnts.begin(), gEnts.end() );
}

void physics_simulation::handle_event( const SDL_Event& e )
{
    physics_app_t::handle_event( e );

    if ( e.type ==  SDL_KEYDOWN )
    {
        switch ( e.key.keysym.sym )
        {
            case SDLK_v:
                gFire = !gFire;
                break;
            case SDLK_r:
                gEnts = std::move( gen_entities() );
                gFire = false;
                break;
        }
    }
}

