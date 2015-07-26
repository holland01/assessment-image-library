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

	tile_t( void );
};

struct generator_t
{
	std::vector< tile_t > tiles;

    std::vector< const tile_t* > billboards;
    std::vector< const tile_t* > walls;

	rend::texture_t billTexture;

	generator_t( void );

	void SetTile( uint32_t pass, uint32_t x, uint32_t z );

	uint32_t RangeCount( uint32_t x, uint32_t z, uint32_t offsetEnd );

    uint32_t Mod( uint32_t x, uint32_t z ) const;

    void GetEntities( std::vector< const tile_t* >& billboards,
                      std::vector< const tile_t* >& walls,
                      const view::frustum_t& frustum,
                      const view::params_t& viewParams ) const;
};

} // namespace mapgen

