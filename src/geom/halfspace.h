#pragma once

#include "_geom_local.h"
#include "transform_data.h"
#include <bullet3/BulletCollision/CollisionShapes/btBox2dShape.h>
#include <bullet3/LinearMath/btConvexHull.h>
#include <memory>

//-------------------------------------------------------------------------------------------------------
// half_space_t
//-------------------------------------------------------------------------------------------------------

struct imm_draw;

struct physics_entity;

struct halfspace
{
private:
    btTransform mAxes;
    float mDistance;
    std::unique_ptr< btBox2dShape, void ( * )( btBox2dShape* ) > mBox;

public:
	halfspace( void );

	halfspace( const glm::mat3& axes, const glm::vec3& origin, float distance );

    halfspace( const physics_entity& physEnt, const glm::vec3& normal );

    halfspace( const halfspace& c );

    halfspace& operator=( const halfspace& c );

    glm::mat3 axes( void ) const;

    glm::vec3 origin( void ) const;

    glm::vec3 normal( void ) const;

	void draw( imm_draw& drawer ) const;
};

INLINE glm::mat3 halfspace::axes( void ) const
{
    return glm::ext::from_bullet( mAxes.getBasis() );
}

INLINE glm::vec3 halfspace::origin( void ) const
{
    return glm::ext::from_bullet( mAxes.getOrigin() );
}

INLINE glm::vec3 halfspace::normal( void ) const
{
    return glm::ext::from_bullet( mAxes.getBasis()[ 2 ] );
}
