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

	const int32_t GEN_PASS_COUNT = 5;

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
	  x( 0 ), z( 0 ),
	  halfSpaceIndex( -1 )
{
}

generator_t::generator_t( void )
	: halfSpaceNormals( {
			glm::vec3( -1.0f, 0.0f, 0.0f ),
			glm::vec3( 0.0f, 0.0f, -1.0f ),
			glm::vec3( 1.0f, 0.0f, 0.0f ),
			glm::vec3( 0.0f, 0.0f, 1.0f )
	  } )
{
	billTexture.mipmap = true;
	billTexture.LoadFromFile( "asset/mooninite.png" );
	billTexture.Load2D();

	tiles.resize( GRID_SIZE * GRID_SIZE );

	// used to compute half-spaces
	std::vector< tile_t* > mutWalls;

	for ( uint32_t pass = 0; pass < GEN_PASS_COUNT; ++pass )
	{
		for ( uint32_t z = 0; z < GRID_SIZE; ++z )
		{
			for ( uint32_t x = 0; x < GRID_SIZE; ++x )
			{
				SetTile( pass, x, z, mutWalls );
			}
		}
	}

	// Iterate through each wall; any adjacent
	// tiles which aren't also walls will allow
	// for this wall to have a half-space
	// for the corresponding face.
	for ( tile_t* wall: mutWalls )
	{
		// NOTE: While most of the generation appears to work, some half-spaces are being
		// produced in areas they shouldn't be...

		std::array< int8_t, NUM_FACES > halfSpaces;
		halfSpaces.fill( 0 );

		int32_t left = TileModIndex( wall->x - 1, wall->z );
		int32_t forward = TileModIndex( wall->x, wall->z - 1 );
		int32_t right = TileModIndex( wall->x + 1, wall->z );
		int32_t back = TileModIndex( wall->x, wall->z + 1 );

		bool hasHalfSpace = false;

		if ( tiles[ left ].type != tile_t::WALL && ( wall->x - 1 ) >= 0 )
		{
			halfSpaces[ FACE_LEFT ] = 1;
			hasHalfSpace = true;
		}

		if ( tiles[ forward ].type != tile_t::WALL && ( wall->z - 1 ) >= 0 )
		{
			halfSpaces[ FACE_FORWARD ] = 1;
			hasHalfSpace = true;
		}

		if ( tiles[ right ].type != tile_t::WALL && ( wall->x + 1 ) < GRID_SIZE )
		{
			halfSpaces[ FACE_RIGHT ] = 1;
			hasHalfSpace = true;
		}

		if ( tiles[ back ].type != tile_t::WALL && ( wall->z + 1 ) < GRID_SIZE )
		{
			halfSpaces[ FACE_BACK ] = 1;
			hasHalfSpace = true;
		}

		if ( hasHalfSpace )
		{
			wall->halfSpaceIndex = halfSpaceTable.size();
			halfSpaceTable.push_back( std::move( halfSpaces ) );
		}
	}
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
			uint32_t i = TileModIndex( ix, iz );

			if ( tiles[ i ].type != tile_t::EMPTY )
			{
				count++;
			}
		}
	}

	return count;
}

void generator_t::SetTile( uint32_t pass, uint32_t x, uint32_t z , std::vector<tile_t*>& mutWalls )
{
	bool isWall;
	uint32_t center = TileIndex( x, z );
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
			mutWalls.push_back( &tiles[ center ] );
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

	//if ( tiles[ center ].type != tile_t::EMPTY )
	{
		glm::mat4 s( glm::scale( glm::mat4( 1.0f ), glm::vec3( size ) ) );
		glm::mat4 t( glm::translate( glm::mat4( 1.0f ), glm::vec3( 2.0f * x, 0.0f, 2.0f * z ) ) );

		tiles[ center ].bounds.reset( new geom::bounding_box_t(
													 glm::vec3( 1.0f ),
													 glm::vec3( -1.0f ),
													 t * s,
													 true ) );
	}
}

namespace {
	bool Predicate_TileCollide( float value )
	{
		return value <= 0.0f && value >= -1.0f;
	}
}

bool generator_t::CollidesWall( const tile_t& t,
								const geom::bounding_box_t& bounds,
								glm::vec3& outNormal )
{
	if ( t.halfSpaceIndex < 0 )
	{
		return false;
	}

	const std::array< int8_t, NUM_FACES >& halfSpaceFaces = halfSpaceTable[ t.halfSpaceIndex ];

	glm::vec3 upAxis( t.bounds->transform[ 1 ] );
	glm::vec3 boundsOrigin( t.bounds->transform[ 3 ] );
	glm::vec3 halfSize( t.bounds->GetSize() * 0.5f );

	for ( uint32_t i = 0; i < halfSpaceFaces.size(); ++i )
	{
		// Find the matrix of the half-space.
		// offset its translation in the direction of the vector MULTIPLIED
		// by the bounds size vector
		if ( halfSpaceFaces[ i ] )
		{
			geom::half_space_t hs;

			hs.extents[ 0 ] = std::move( glm::cross( halfSpaceNormals[ i ], upAxis ) );
			hs.extents[ 1 ] = upAxis;
			hs.extents[ 2 ] = halfSpaceNormals[ i ];
			hs.origin = std::move( boundsOrigin + halfSpaceNormals[ i ] * halfSize );
			hs.distance = glm::dot( hs.extents[ 2 ], hs.origin );

			if ( bounds.IntersectsHalfSpace( hs ) )
			{
				outNormal = halfSpaceNormals[ i ];
				return true;
			}
		}
	}

	return false;
}

void generator_t::GetEntities( std::vector< const tile_t* >& outBillboards,
							   std::vector< const tile_t* >& outWalls,
							   std::vector< const tile_t* >& outFreeSpace,
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

			int32_t index = TileModIndex( x, z );

			if ( !frustum.IntersectsBox( *tiles[ index ].bounds ) )
			{
				continue;
			}

			switch ( tiles[ index ].type )
			{
				case tile_t::BILLBOARD:
					outBillboards.push_back( &tiles[ index ] );
					break;
				case tile_t::WALL:
					outWalls.push_back( &tiles[ index ] );
					break;
				case tile_t::EMPTY:
					outFreeSpace.push_back( &tiles[ index ] );
					break;
			}
			// cull frustum, insert into appropriate type, etc.
		}
	}

	// do view-space depth sort on each vector
}

} // namespace map
