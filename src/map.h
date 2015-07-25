#pragma once

#include "def.h"
#include "geom.h"
#include "renderer.h"

namespace map {

struct tile_t
{
	std::unique_ptr< geom::bounding_box_t > bounds;
	std::unique_ptr< rend::billboard_t > billboard;

	tile_t( void );
};

struct generator_t
{
	std::vector< tile_t > tiles;

	rend::texture_t billTexture;

	generator_t( void );

	void SetTile( uint32_t pass, uint32_t x, uint32_t z );

	uint32_t RangeCount( uint32_t x, uint32_t z, uint32_t offsetEnd );

	uint32_t Mod( uint32_t x, uint32_t z );
};

} // namespace mapgen

