#pragma once

#include "_geom_local.h"

struct imm_draw;

struct ray
{
    glm::vec3 mOrigin;
    glm::vec3 mDir;
    float mT;

	ray( const glm::vec3& position = glm::vec3( 0.0f ),
		 const glm::vec3& dir = glm::vec3( 0.0f ),
         float t_ = 1.0f );

    ray( const ray& r );

    ray& operator=( const ray& r );

    FORCEINLINE glm::vec3 calc_position( float t ) const { return mOrigin + mDir * t; }

    FORCEINLINE glm::vec3 calc_position( void ) const { return calc_position( mT ); }

    void draw( imm_draw& drawer, const glm::vec4& color = glm::vec4( 1.0f ) ) const;
};

