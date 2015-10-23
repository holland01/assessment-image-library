#include "physics_body.h"
#include "renderer.h"
#include "view.h"
#include "physics_world.h"

namespace {
INLINE float mass_from_body( const btRigidBody* body )
{
    float imass = body->getInvMass();
    if ( imass == 0.0f )
        return imass;

    return 1.0f / imass;
}

INLINE void toggle_body_flag( bool& flip, btRigidBody* body,
                              int32_t flag, int32_t forceActivateOn = DISABLE_DEACTIVATION, int32_t setActivateOff = 0 )
{
    if ( !body )
        return;

    flip = !flip;

    if ( flip )
    {
        body->setCollisionFlags( body->getCollisionFlags() | flag );
        body->forceActivationState( forceActivateOn );
    }
    else
    {
        body->setCollisionFlags( body->getCollisionFlags() & ~flag );
        body->setActivationState( setActivateOff );
    }
}

}

physics_world* physics_body::mWorldPtr = nullptr;

void physics_body::set_physics_world( physics_world& world )
{
    mWorldPtr = &world;
}

physics_body::physics_body( btCollisionShape* shape, btMotionState* ms, btRigidBody* body )
    : mOwned( false ),
      mKinematic( false ), mStatic( false ),
      mShape( shape ),
      mMotionState( ms ),
      mBody( body )
{
}

physics_body::physics_body( float mass, const glm::mat4& orientAndTranslate, const glm::vec3& halfSpaceExtents )
    : mOwned( false ),
      mKinematic( false ), mStatic( false ),
      mShape( new btBoxShape( glm::ext::to_bullet( halfSpaceExtents ) ) ),
      mMotionState( new btDefaultMotionState( glm::ext::to_bullet( orientAndTranslate ) ) ),
      mBody( new btRigidBody( btRigidBody::btRigidBodyConstructionInfo( mass, mMotionState.get(), mShape.get() ) ) )
{
}

physics_body::physics_body( void )
    : physics_body( nullptr, nullptr, nullptr )
{
}

float physics_body::mass( void )
{
    return mass_from_body( mBody.get() );
}

void physics_body::iterate_collision_points( collision_point_fn_t callback ) const
{
    UNUSEDPARAM( callback ); //TODO: implement this
}

void physics_body::draw( imm_draw& drawer, uint32_t primType ) const
{
    // Must be lesss than this value to have access to getVertex/getNumVertices functions
    if ( mShape->getShapeType() >= ( int32_t )IMPLICIT_CONVEX_SHAPES_START_HERE )
    {
        return;
    }

    const btPolyhedralConvexShape& shapeAsPolyhedral = *( ( const btPolyhedralConvexShape* )mShape.get() );

    btTransform t;
    mMotionState->getWorldTransform( t );

    const btVector3& origin = t.getOrigin();
    const btMatrix3x3& basis = t.getBasis();

    drawer.begin( ( GLenum )primType );
    for ( int32_t i = 0; i < shapeAsPolyhedral.getNumVertices(); ++i )
    {
        btVector3 v;
        shapeAsPolyhedral.getVertex( i, v );

        v = origin + basis * v;
        glm::vec3 vg( glm::ext::from_bullet( v ) );

        drawer.vertex( vg );
    }
    drawer.end();
}

void physics_body::draw( const std::string& buffer,
                           const std::string& program,
                           const view_data& view,
                           const glm::vec4& color ) const
{
    bind_program pbind( program );
    pbind.program().load_mat4( "modelToView", view.mTransform * world_transform() );
    pbind.program().load_vec4( "color", color );

    bind_buffer bbuff( buffer );
    bbuff.buffer().render( pbind.program() );
}

void physics_body::toggle_kinematic( void )
{
    toggle_body_flag( mKinematic, mBody.get(), btRigidBody::CF_KINEMATIC_OBJECT );
}

void physics_body::toggle_static( void )
{
    toggle_body_flag( mStatic, mBody.get(), btRigidBody::CF_STATIC_OBJECT, 0, 0 );
}

void physics_body::add_to_world( void ) const
{
    assert( mWorldPtr );

    if ( !mOwned )
    {
        mWorldPtr->mPhysEntities.push_back( this );
        mWorldPtr->mDynamics->addRigidBody( mBody.get() );
        mOwned = true;
    }
}

void physics_body::remove_from_world( void ) const
{
    assert( mWorldPtr );

    if ( mOwned )
    {
        vector_remove_ptr( mWorldPtr->mPhysEntities, this );

        mWorldPtr->mDynamics->removeRigidBody( mBody.get() );
        mOwned = false;
    }
}

void physics_body::orient_to( const glm::vec3& v )
{
    btTransform worldTrans;
    mMotionState->getWorldTransform( worldTrans );

    glm::vec3 boundsOrigin( glm::ext::from_bullet( worldTrans.getOrigin() ) );

    glm::vec3 dir( v - boundsOrigin );
    dir.y = 0.0f;
    dir = glm::normalize( dir );

    glm::mat3 orient(
        orient_by_direction(
                dir,
                glm::vec3( 0.0f, 0.0f, 1.0f ),
                glm::vec3( -1.0f, 0.0f, 0.0f )
        )
    );

    glm::quat q( glm::quat_cast( orient ) );
    worldTrans.setRotation( glm::ext::to_bullet( q ) );
    mMotionState->setWorldTransform( worldTrans );
}
