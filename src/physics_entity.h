#pragma once

#include "def.h"

#include <memory>
#include <bullet3/btBulletCollisionCommon.h>
#include <bullet3/btBulletDynamicsCommon.h>
#include <functional>
#include <vector>
#include "glm_ext.hpp"
#include <glm/gtc/matrix_transform.hpp>

struct imm_draw;
struct view_data;
struct physics_world;

struct physics_entity
{
protected:
    friend struct entity;
    friend struct input_client;

private:
    mutable bool mOwned;

    std::unique_ptr< btCollisionShape > mShape;

    std::unique_ptr< btDefaultMotionState > mMotionState;

    std::unique_ptr< btRigidBody > mBody;

    using collision_point_fn_t = std::function< void( const btVector3& wp ) >;

public:
    physics_entity( btCollisionShape* shape, btDefaultMotionState* ms, btRigidBody* body );

    physics_entity( float mass, const glm::mat4& orientAndTranslate, const glm::vec3& halfSpaceExtents );

    physics_entity( void );

    const btBoxShape* shape_as_box( void ) const { return ( const btBoxShape* ) mShape.get(); }

    const btPolyhedralConvexShape* shape_as_polyhedral( void ) const { return ( const btPolyhedralConvexShape* )mShape.get(); }

    const btDefaultMotionState& motion_state( void ) const { return *mMotionState; }

    glm::mat4 world_transform( void ) const;

    std::vector< glm::vec3 > world_space_points( void ) const;

    void draw( imm_draw& drawer, uint32_t primType ) const;

    void draw( const std::string& buffer,
               const std::string& program,
               const view_data& view,
               const glm::vec4& color ) const;

    void add_to_world( physics_world& world ) const;

    void remove_from_world( physics_world& world ) const;

    void iterate_collision_points( collision_point_fn_t callback ) const;
};

INLINE glm::mat4 physics_entity::world_transform( void ) const
{
    assert( mMotionState );

    btTransform t;
    mMotionState->getWorldTransform( t );

    glm::mat4 m( glm::ext::from_bullet( t ) );

    if ( mShape->getShapeType() == BOX_SHAPE_PROXYTYPE )
    {
        const btBoxShape* shape = shape_as_box();

        glm::vec3 halfExtents( glm::ext::from_bullet( shape->getHalfExtentsWithMargin() ) );

        m[ 0 ] *= glm::vec4( halfExtents.x, halfExtents.x, halfExtents.x, 0.0f );
        m[ 1 ] *= glm::vec4( halfExtents.y, halfExtents.y, halfExtents.y, 0.0f );
        m[ 2 ] *= glm::vec4( halfExtents.z, halfExtents.z, halfExtents.z, 0.0f );
    }

    return m;
}

INLINE std::vector< glm::vec3 > physics_entity::world_space_points( void ) const
{
    const btPolyhedralConvexShape* thisShape = shape_as_polyhedral();

    std::vector< glm::vec3 > points;
    points.reserve( thisShape->getNumVertices() );

    btTransform t;
    mMotionState->getWorldTransform( t );

    for ( int32_t i = 0; i < thisShape->getNumVertices(); ++i )
    {
        btVector3 v;
        thisShape->getVertex( i, v );

        points.push_back( glm::ext::from_bullet( t.getOrigin() + t.getBasis() * v ) );
    }

    return std::move( points );
}
