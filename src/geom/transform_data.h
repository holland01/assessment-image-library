#pragma once

#include "../def.h"
#include "../glm_ext.hpp"

struct transform_data
{
    glm::mat3 mAxes;
    glm::vec3 mExtents;
    glm::vec3 mOrigin;

    transform_data( const glm::mat3 axes = glm::mat3( 1.0f ),
                    const glm::vec3 extents = glm::vec3( 1.0f ),
                    const glm::vec3 origin = glm::vec3( 0.0f ) )
        : mAxes( std::move( axes ) ),
          mExtents( std::move( extents ) ),
          mOrigin( std::move( origin ) )
    {
    }

    glm::mat4 world_transform( void ) const;

    glm::mat3 scaled_axes( void ) const { return std::move( glm::ext::scale( mAxes, mExtents ) ); }
};

INLINE glm::mat4 transform_data::world_transform( void ) const
{
    glm::mat4 t( glm::ext::scale( mAxes, mExtents ) );
    t[ 3 ] = glm::vec4( mOrigin, 1.0f );
    return std::move( t );
}
