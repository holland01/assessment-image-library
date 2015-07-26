#include "map.h"
#include <random>
#include <glm/gtc/matrix_transform.hpp>

namespace {
	std::random_device randDevice;
	std::mt19937 randEngine( randDevice() );
	std::uniform_int_distribution< uint32_t > wallDet( 0, 100 );
	std::uniform_int_distribution< uint16_t > randByte( 0, 255 );

	glm::vec4 RandomColor( void )
	{
		std::uniform_real_distribution< float > color( 0.5f, 1.0f );
		return glm::vec4( color( randEngine ),
						  color( randEngine ),
						  color( randEngine ),
						  1.0f );
	}

	const uint32_t GRID_SIZE = 100;

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
	: bounds( nullptr )
{
}

generator_t::generator_t( void )
{
	billTexture.LoadFromFile( "asset/mooninite.png" );

	/*
	billTexture.SetBufferSize( 256, 256, 4, 255 );

	std::array< uint8_t, 8 > blackWhite = 
	{
		0x00, 0x00, 0x00, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF
	};

	for ( size_t i = 0; i < billTexture.pixels.size(); i += 4  )
	{
		billTexture.pixels[ i + 0 ] = blackWhite[ ( i + 0 ) % blackWhite.size() ]; //randByte( randEngine );
		billTexture.pixels[ i + 1 ] = blackWhite[ ( i + 1 ) % blackWhite.size() ]; //randByte( randEngine );;
		billTexture.pixels[ i + 2 ] = blackWhite[ ( i + 2 ) % blackWhite.size() ]; //randByte( randEngine );;
		billTexture.pixels[ i + 3 ] = blackWhite[ ( i + 3 ) % blackWhite.size() ]; //128;
	}
	*/
	billTexture.Load2D();

	tiles.resize( GRID_SIZE * GRID_SIZE );

	for ( uint32_t pass = 0; pass < 5; ++pass )
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

uint32_t generator_t::Mod( uint32_t x, uint32_t z )
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
		isWall = wallDet( randEngine ) <= 45u;
	}
	else
	{
		isWall = RangeCount( x - 1, z - 1, 3 ) >= 5 || predicates[ pass - 1 ]( RangeCount( x - 2, z - 2, 5 ) );
	}

	tiles[ center ].bounds.reset( nullptr );
	tiles[ center ].billboard.reset( nullptr );

	if ( isWall )
	{
		tiles[ center ].bounds.reset( new geom::bounding_box_t(
													 glm::vec3( 1.0f ),
													 glm::vec3( -1.0f ),
													 glm::translate( glm::scale( glm::mat4( 1.0f ), glm::vec3( size ) ),
																	 glm::vec3( x * 2.0f, 0.0f, z * 2.0f ) ),
													 true ) );

		tiles[ center ].bounds->SetDrawable( RandomColor() );
	}
	else
	{
		bool isSprite = wallDet( randEngine ) <= 5u;
		if ( isSprite )
		{
			tiles[ center ].billboard.reset( new rend::billboard_t( glm::vec3( x * 2.0f, 0.0f, z * 2.0f ),
																	billTexture ) );
		}
	}
}

} // namespace map
