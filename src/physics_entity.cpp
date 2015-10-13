#include "physics_entity.h"
#include "renderer.h"
#include "view.h"

physics_entity::physics_entity( btCollisionShape* shape, btDefaultMotionState* ms, btRigidBody* body )
    : mOwned( false ),
      mShape( shape ),
      mMotionState( ms ),
      mBody( body )
{
}

physics_entity::physics_entity( float mass, const glm::mat4& orientAndTranslate, const glm::vec3& halfSpaceExtents )
    : mShape( new btBoxShape( glm::ext::to_bullet( halfSpaceExtents ) ) ),
      mMotionState( new btDefaultMotionState( glm::ext::to_bullet( orientAndTranslate ) ) ),
      mBody( new btRigidBody( btRigidBody::btRigidBodyConstructionInfo( mass, mMotionState.get(), mShape.get() ) ) )
{
}

physics_entity::physics_entity( void )
    : physics_entity( nullptr, nullptr, nullptr )
{
}

void physics_entity::draw( imm_draw& drawer, uint32_t primType ) const
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

void physics_entity::draw( const std::string& buffer,
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
