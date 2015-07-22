#include "map.h"

namespace map {

area_t::area_t( const glm::vec3 &dims, const glm::mat3 &transform_, const glm::vec3 &origin_, uint32_t count )
	: transform( transform_ ),
	  origin( origin_ )
{
	const glm::vec3 halfDims( dims * 0.5f );

	boundsList.reserve( count );
	for ( uint32_t i = 0; i < count; ++i )
	{
        glm::vec3 center( origin + glm::vec3( 0.0f, 0.0f, dims.z * ( float ) i + 5.0f ) );

		glm::vec3 max( center + halfDims );
		glm::vec3 min( center - halfDims );

		geom::bounding_box_t bounds( max, min, transform, true );
		bounds.SetDrawable( glm::u8vec4( 128, 0, 255, 255 ) );
		boundsList.push_back( std::move( bounds ) );
	}
}

} // namespace map
