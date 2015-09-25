#pragma once

#include "../def.h"
#include "../glm_ext.hpp"
#include <glm/gtc/quaternion.hpp>

namespace {
	// eliminate scaling through normalization of quat
	INLINE glm::mat3 convert_axes( const glm::mat4& t )
	{
		return std::move( glm::mat3_cast( glm::normalize( glm::quat_cast( t ) ) ) );
	}

	// remove rotation ( sort of )
	INLINE glm::vec3 convert_extents( const glm::mat4& t )
	{
		 return glm::ceil( glm::vec3( t[ 0 ][ 0 ], t[ 1 ][ 1 ], t[ 2 ][ 2 ] ) );
	}
}

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

	// Hacky means of converting a mat4 to a
	// transform_data format. Use this ctor
	// only if you know what you're doing...
	transform_data( const glm::mat4& t )
		: mAxes( convert_axes( t ) ),
		  mExtents( convert_extents( t ) ),
		  mOrigin( t[ 3 ] )
	{
	}

    glm::mat4 world_transform( void ) const;

    glm::mat3 scaled_axes( void ) const { return std::move( glm::ext::scale( mAxes, mExtents ) ); }

	transform_data transform_to( const transform_data& dest ) const;
};

INLINE glm::mat4 transform_data::world_transform( void ) const
{
    glm::mat4 t( glm::ext::scale( mAxes, mExtents ) );
    t[ 3 ] = glm::vec4( mOrigin, 1.0f );
    return std::move( t );
}

INLINE transform_data transform_data::transform_to( const transform_data& dest ) const
{
	return std::move( transform_data( dest.mAxes * mAxes,
									  dest.mExtents * mExtents,
									  dest.mOrigin + mOrigin ) );
}
