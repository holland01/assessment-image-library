#include "map.h"
#include <random>
#include <glm/gtc/matrix_transform.hpp>

namespace {
	std::random_device randDevice;
	std::mt19937 randEngine( randDevice() );
}

namespace map {

area_t::area_t(
		const glm::vec3 &dims,
		const glm::mat4 &model_,
		const glm::mat4& world_,
		const glm::vec3 &origin_,
		uint32_t count )
	: model( model_ ),
	  world( world_ )
{
	const glm::vec3 halfDims( dims * 0.5f );

	std::uniform_real_distribution< float > color( 0.5f, 1.0f );

	boundsList.reserve( count );
	for ( uint32_t i = 0; i < count; ++i )
	{
		glm::vec3 center( origin_ + glm::vec3( 0.0f, 0.0f, ( float ) i * dims.z ) );

		glm::mat4 t( world * glm::scale( model * glm::translate( glm::mat4( 1.0f ), center ), halfDims ) );

		geom::bounding_box_t bounds( glm::vec3( 1.0f ), glm::vec3( -1.0f ), t, true );
		bounds.SetDrawable( glm::vec4( color( randEngine ), color( randEngine ), color( randEngine ), 0.1f ) );
		boundsList.push_back( std::move( bounds ) );
	}
}

area_t::area_t( area_t&& m )
	: model( std::move( m.model ) ),
	  world( std::move( m.world ) ),
	  boundsList( std::move( m.boundsList ) )
{
}

generator_t::generator_t( void )
{
	std::uniform_real_distribution< float > angle( -45.0f, 45.0f );
	std::uniform_int_distribution< int > axis( 0, 1 );

	const uint32_t NUM_AREAS = 5;
	const uint32_t NUM_BOUNDS = 5;
	glm::vec3 dims( 20.0f );
	glm::vec3 lastOrigin( 0.0f );

	for ( uint32_t i = 0; i < NUM_AREAS; ++i )
	{
		glm::mat4 model( glm::rotate( glm::mat4( 1.0f ), glm::radians( angle( randEngine ) ), glm::vec3( 0.0f, 1.0f, 0.0f ) ) );

		area_t area( dims,
					 model,
					 glm::translate( glm::mat4( 1.3f ), lastOrigin ),
					 glm::vec3( 0.0f ),
					 NUM_BOUNDS );

		lastOrigin = glm::vec3( area.boundsList[ NUM_BOUNDS - 1 ].transform[ 3 ] );

		areas.push_back( std::move( area ) );

	}
}

} // namespace map
