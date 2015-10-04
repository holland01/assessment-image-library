#include "physics_simulation.h"
#include "debug_app.h"
#include "application_update.h"
#include <bullet3/btBulletCollisionCommon.h>
#include <bullet3/btBulletDynamicsCommon.h>

using namespace std;

#define GROUND_MASS 10000.0f

namespace {

    const glm::vec4 G_TAG_COLOR( 0.0f, 0.0f, 1.0f, 1.0f );

    vector< entity* > gen_entities( void )
    {
        return std::vector< entity* >();
    }

    vector< entity* > gEnts = gen_entities();

    struct physics_sys
    {
        using rigid_info_t = btRigidBody::btRigidBodyConstructionInfo;

        unique_ptr< btBroadphaseInterface > mBroadphase;
        unique_ptr< btDefaultCollisionConfiguration > mCollisionConfig;
        unique_ptr< btCollisionDispatcher > mDispatcher;
        unique_ptr< btSequentialImpulseConstraintSolver > mSolver;
        unique_ptr< btDiscreteDynamicsWorld > mDynamics;
        unique_ptr< btCollisionShape > mGroundShape, mFallShape;
        unique_ptr< btDefaultMotionState > mGroundMotionState, mFallMotionState, mFall2MotionState;
        unique_ptr< btRigidBody > mGroundRigidBody, mFallRigidBody, mFall2RigidBody;
        unique_ptr< btSliderConstraint > mSliderConst;

        struct wall
        {
            unique_ptr< btCollisionShape > mShape;
            unique_ptr< btRigidBody > mBody;
            unique_ptr< btDefaultMotionState > mMotionState;
        };

        std::array< wall, 4 > mWalls;

        physics_sys( void )
            : mBroadphase( new btDbvtBroadphase() ),
              mCollisionConfig( new btDefaultCollisionConfiguration() ),
              mDispatcher( new btCollisionDispatcher( mCollisionConfig.get() ) ),
              mSolver( new btSequentialImpulseConstraintSolver() ),
              mDynamics( new btDiscreteDynamicsWorld( mDispatcher.get(),
                                                      mBroadphase.get(),
                                                      mSolver.get(),
                                                      mCollisionConfig.get() ) ),
              mGroundShape( new btStaticPlaneShape( btVector3( 0, 1, 0 ), 1 ) ),
              mFallShape( new btSphereShape( 1 ) ),

              mGroundMotionState( new btDefaultMotionState( btTransform( btQuaternion( 0, 0, 0, 1 ),
                                                                         btVector3( 0, -1, 0 ) ) ) ),

              mFallMotionState( new btDefaultMotionState( btTransform( btQuaternion( 0, 0, 0, 1 ),
                                                                       btVector3( 0, 100, 0 ) ) ) ),

              mFall2MotionState( new btDefaultMotionState( btTransform( btQuaternion( 0, 0, 0, 1 ),
                                                                        btVector3( 8, 100, 0 ) ) ) ),

              mGroundRigidBody( new btRigidBody( rigid_info_t( 0,
                                                               mGroundMotionState.get(),
                                                               mGroundShape.get(),
                                                               btVector3( 0, 0, 0 ) ) ) )
        {
            mDynamics->setGravity( btVector3( 0, -1.0f, 0 ) );
            mDynamics->setSynchronizeAllMotionStates( true );

            mDynamics->addRigidBody( mGroundRigidBody.get() );

            btScalar mass = 1;
            btVector3 fallInertia( 10, 10, 10 );
            mFallShape->calculateLocalInertia( mass, fallInertia );

            mFallRigidBody.reset( new btRigidBody( rigid_info_t( mass,
                                                                 mFallMotionState.get(),
                                                                 mFallShape.get(),
                                                                 fallInertia ) ) );

            mFall2RigidBody.reset( new btRigidBody( rigid_info_t( mass,
                                                                  mFall2MotionState.get(),
                                                                  mFallShape.get(),
                                                                  fallInertia ) ) );

            mDynamics->addRigidBody( mFallRigidBody.get() );
            mDynamics->addRigidBody( mFall2RigidBody.get() );

            //btVector3 a, b;
            btTransform a, b;
            //a.setValue( 0, 0, 0 );
            //b.setValue( 0, 0, 0 );
            a.setIdentity();
            b.setIdentity();

            mSliderConst.reset( new btSliderConstraint( *mFallRigidBody, *mFall2RigidBody, a, b, true ) );
            //mSliderConst->m_setting.m_damping = 0.3f;
            mDynamics->addConstraint( mSliderConst.get() );

            struct plane_params
            {
                btVector3 normal;
                btVector3 origin;
                float d;
            };

            float dist = 10.0f;
            float d = 10.0f;
            // order is left, forward, right, back
            std::array< plane_params, 4 > params =
            {{
                { btVector3( 1.0f, 0.0f, 0.0f ), btVector3( -dist, 0.0f, 0.0f ), d },
                { btVector3( 0.0f, 0.0f, 1.0f ), btVector3( 0.0f, 0.0f, -dist ), d },
                { btVector3( -1.0f, 0.0f, 0.0f ), btVector3( dist, 0.0f, 0.0f ), -d },
                { btVector3( 0.0f, 0.0f, -1.0f ), btVector3( 0.0f, 0.0f, dist ), -d }
            }};

            for ( uint32_t i: { 0, 1, 2, 3 } )
            {
                mWalls[ i ].mShape.reset( new btStaticPlaneShape( params[ i ].normal, params[ i ].d ) );
                mWalls[ i ].mMotionState.reset( new btDefaultMotionState( btTransform( btQuaternion( 0, 0, 0, 1 ),
                                                                                       btVector3( params[ i ].origin ) ) ) );
                mWalls[ i ].mBody.reset( new btRigidBody( rigid_info_t( 0, mWalls[ i ].mMotionState.get(),
                                                                        mWalls[ i ].mShape.get(),
                                                                        btVector3( 0, 0, 0 ) ) ) );

                mDynamics->addRigidBody( mWalls[ i ].mBody.get() );
            }
        }
    };

    physics_sys gSystem;
}

physics_simulation::physics_simulation( uint32_t w, uint32_t h )
    : physics_app_t( w, h )
{
    camera = &spec;
    camera->position( glm::vec3( 0.0f, 10.0f, 100.0f ) );
}

void physics_simulation::frame( void )
{
    const view_data& vp = camera->view_params();

    frustum.update( vp );

    world.update( *this );

    gSystem.mDynamics->stepSimulation( 1.0f / 60.0f, 10 );
    application_frame< physics_simulation > theFrame( *this );
}

void physics_simulation::draw( void )
{
    bind_program bound_prog( "single_color" );
    const draw_buffer& cubeDraw = pipeline->buffer( "colored_cube" );

    auto draw_it = [ & ]( const btMotionState* state, const glm::vec4& color )
    {
        btTransform T;
        state->getWorldTransform( T );
        glm::mat4 rT;
        T.getOpenGLMatrix( &rT[ 0 ][ 0 ] );

        bound_prog.program().load_vec4( "color", color );
        bound_prog.program().load_mat4( "modelToView", camera->view_params().mTransform * rT );
        cubeDraw.render( bound_prog.program() );
    };

    cubeDraw.bind();
    draw_it( gSystem.mFallMotionState.get(), glm::vec4( 1.0f, 0.0f, 0.0f, 1.0f ) );
    draw_it( gSystem.mFall2MotionState.get(), glm::vec4( 0.0f, 1.0f, 0.0f, 1.0f ) );
    cubeDraw.release();

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
        // TODO
    }
}

