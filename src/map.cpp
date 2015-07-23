#include "map.h"

#include <glm/gtc/matrix_transform.hpp>

namespace map {

area_t::area_t( const glm::vec3 &dims, const glm::mat4 &transform_, const glm::vec3 &origin_, uint32_t count )
	: transform( transform_ ),
	  origin( origin_ )
{
	const glm::vec3 halfDims( dims * 0.5f );
	//glm::vec3 forward( glm::normalize( transform[ 2 ] ) );

	boundsList.reserve( count );
	for ( uint32_t i = 0; i < count; ++i )
	{
		glm::vec3 center( origin + glm::vec3( 0.0f, 0.0f, ( float ) i * dims.z ) );

		glm::mat4 t( glm::scale( transform * glm::translate( glm::mat4( 1.0f ), center ), dims * 0.5f ) );

		geom::bounding_box_t bounds( halfDims, -halfDims, t, true );
		bounds.SetDrawable( glm::u8vec4( 128, 0, 255, 255 ) );
		boundsList.push_back( std::move( bounds ) );
	}
}

} // namespace map
