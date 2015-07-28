#pragma once

#include "def.h"
#include "geom.h"
#include "renderer.h"

namespace view {
    struct frustum_t;
}

namespace map {

struct tile_t
{
    enum type_t
    {
        BILLBOARD,
        WALL,
        EMPTY
    };

	std::unique_ptr< geom::bounding_box_t > bounds;
    type_t type;
	int32_t x, z, halfSpaceIndex;

	tile_t( void );
};

struct generator_t
{	
	static const int32_t GRID_SIZE = 100;

	enum faceIndex_t
	{
		FACE_LEFT = 0,
		FACE_FORWARD,
		FACE_RIGHT,
		FACE_BACK,
		NUM_FACES
	};

	std::vector< tile_t > tiles;

    std::vector< const tile_t* > billboards;
    std::vector< const tile_t* > walls;
	std::vector< const tile_t* > freeSpace;

	rend::texture_t billTexture;

	std::vector< std::array< int8_t, NUM_FACES > > halfSpaceTable;
	std::array< glm::vec3, NUM_FACES > halfSpaceNormals;

	generator_t( void );

	void SetTile( uint32_t pass,
				  uint32_t x,
				  uint32_t z,
				  std::vector< tile_t* >& mutWalls );

	uint32_t RangeCount( uint32_t x, uint32_t z, uint32_t offsetEnd );

	uint32_t TileIndex( uint32_t x, uint32_t z ) const;
	uint32_t TileModIndex( uint32_t x, uint32_t z ) const;

	bool CollidesWall( const tile_t* t, const geom::bounding_box_t& bounds, glm::vec3& outNormal );

	void GetEntities( std::vector< const tile_t* >& billboards,
					  std::vector< const tile_t* >& walls,
					  std::vector< const tile_t* >& freeSpace,
					  const view::frustum_t& frustum,
					  const view::params_t& viewParams ) const;
};

INLINE uint32_t generator_t::TileIndex( uint32_t x, uint32_t z ) const
{
	return z * GRID_SIZE + x;
}

INLINE uint32_t generator_t::TileModIndex( uint32_t x, uint32_t z ) const
{
	return ( z % GRID_SIZE ) * GRID_SIZE + x % GRID_SIZE;
}

} // namespace mapgen

