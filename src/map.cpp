#include "map.h"
#include "view.h"
#include <random>
#include <glm/gtc/matrix_transform.hpp>

namespace {
	std::random_device randDevice;
	std::mt19937 randEngine( randDevice() );
	std::uniform_int_distribution< uint32_t > wallDet( 0, 100 );
	std::uniform_int_distribution< uint16_t > randByte( 0, 255 );

	INLINE glm::vec4 RandomColor( void )
	{
		std::uniform_real_distribution< float > color( 0.5f, 1.0f );
		return glm::vec4( color( randEngine ),
						  color( randEngine ),
						  color( randEngine ),
						  1.0f );
	}

	const uint32_t GRID_SIZE = 100;
	const uint32_t GEN_PASS_COUNT = 5;

	std::array< std::function< bool( uint32_t ) >, 4 > predicates =
	{{
		[]( uint32_t c )
		{
			return c <= 1;
		},
		[]( uint32_t c )
		{
			return c <= 2;
		},
		[]( uint32_t c )
		{
			UNUSEDPARAM( c );
			return false;
		},
		[]( uint32_t c )
		{
			UNUSEDPARAM( c );
			return false;
		}
	}};
}

namespace map {

tile_t::tile_t( void )
	: bounds( nullptr ),
	  type( tile_t::EMPTY ),
	  x( 0 ), z( 0 )
{
}

generator_t::generator_t( void )
{
	billTexture.mipmap = true;
	billTexture.LoadFromFile( "asset/mooninite.png" );
	billTexture.Load2D();

	tiles.resize( GRID_SIZE * GRID_SIZE );

	for ( uint32_t pass = 0; pass < GEN_PASS_COUNT; ++pass )
	{
		for ( uint32_t z = 0; z < GRID_SIZE; ++z )
		{
			for ( uint32_t x = 0; x < GRID_SIZE; ++x )
			{
				SetTile( pass, x, z );
			}
		}
	}
}

uint32_t generator_t::Mod( uint32_t x, uint32_t z ) const
{
	return ( z % GRID_SIZE ) * GRID_SIZE + x % GRID_SIZE;
}

uint32_t generator_t::RangeCount( uint32_t x, uint32_t z, uint32_t endOffset )
{
	if ( ( int32_t )x < 0 )
	{
		x = 0;
	}

	if ( ( int32_t )z < 0  )
	{
		z = 0;
	}

	uint32_t ex = ( x + endOffset );
	uint32_t ez = ( z + endOffset );

	uint32_t count = 0;
	for ( uint32_t iz = z; iz < ez; ++iz )
	{
		for ( uint32_t ix = x; ix < ex; ++ix )
		{
			uint32_t i = Mod( ix, iz );

			if ( tiles[ i ].bounds )
			{
				count++;
			}
		}
	}

	return count;
}

void generator_t::SetTile( uint32_t pass, uint32_t x, uint32_t z )
{
	bool isWall;
	uint32_t center = z * GRID_SIZE + x;
	const float size = 1.0f;

	if ( pass == 0 )
	{
		isWall = wallDet( randEngine ) <= 40u;
	}
	else
	{
		isWall = RangeCount( x - 1, z - 1, 3 ) >= 5 || predicates[ pass - 1 ]( RangeCount( x - 2, z - 2, 5 ) );
	}

	tiles[ center ].bounds.reset( nullptr );
	tiles[ center ].type = tile_t::EMPTY;
	tiles[ center ].x = x;
	tiles[ center ].z = z;

	// Multiply x and z values by 2 to accomodate for bounds scaling on the axis
	if ( isWall )
	{
		tiles[ center ].type = tile_t::WALL;
		if ( pass == GEN_PASS_COUNT - 1 )
		{
			walls.push_back( &tiles[ center ] );
		}
	}
	else
	{
		// There is a 5% chance that billboards will be added
		if ( wallDet( randEngine ) <= 5u )
		{
			tiles[ center ].type = tile_t::BILLBOARD;
			if ( pass == GEN_PASS_COUNT - 1 )
			{
				billboards.push_back( &tiles[ center ] );
			}

		}
		else
		{
			if ( pass == GEN_PASS_COUNT - 1 )
			{
				freeSpace.push_back( &tiles[ center ] );
			}
		}
	}

	if ( tiles[ center ].type != tile_t::EMPTY )
	{
		glm::mat4 s( glm::scale( glm::mat4( 1.0f ), glm::vec3( size ) ) );
		glm::mat4 t( glm::translate( glm::mat4( 1.0f ), glm::vec3( 2.0f * x, 0.0f, 2.0f * z ) ) );

		tiles[ center ].bounds.reset( new geom::bounding_box_t(
													 glm::vec3( 1.0f ),
													 glm::vec3( -1.0f ),
													 t * s,
													 true ) );

		if ( isWall )
		{
			tiles[ center ].bounds->SetDrawable( RandomColor() );
		}
	}
}

void generator_t::GetEntities( std::vector< const tile_t* >& billboards,
							   std::vector< const tile_t* >& walls,
							   const view::frustum_t& frustum,
							   const view::params_t& viewParams ) const
{
	int32_t centerX = ( int32_t )( viewParams.origin.x * 0.5f );
	int32_t centerZ = ( int32_t )( viewParams.origin.z * 0.5f );

	if ( centerX < 0 || centerZ < 0 )
	{
		return;
	}

	const int32_t RADIUS = 20;
	const int32_t startX = centerX - RADIUS;
	const int32_t startZ = centerZ - RADIUS;
	const int32_t endX = centerX + RADIUS;
	const int32_t endZ = centerZ + RADIUS;

	for ( int32_t z = startZ; z < endZ; ++z )
	{
		if ( z >= ( int32_t ) GRID_SIZE )
		{
			break;
		}

		for ( int32_t x = startX; x < endX; ++x )
		{
			if ( x >= ( int32_t ) GRID_SIZE )
			{
				break;
			}

			int32_t index = Mod( x, z );

			if ( tiles[ index ].type == tile_t::EMPTY )
			{
				continue;
			}

			if ( !frustum.IntersectsBox( *tiles[ index ].bounds ) )
			{
				continue;
			}

			switch ( tiles[ index ].type )
			{
				case tile_t::BILLBOARD:
					billboards.push_back( &tiles[ index ] );
					break;
				case tile_t::WALL:
					walls.push_back( &tiles[ index ] );
					break;
				default:
					break; // prevent compiler from bitching
			}
			// cull frustum, insert into appropriate type, etc.
		}
	}

	// do view-space depth sort on each vector
}

} // namespace map
