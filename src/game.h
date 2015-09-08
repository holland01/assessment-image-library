#pragma once

#include "geom.h"
#include "input.h"
#include "map.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_main.h>
#include <memory>

struct application
{
private:
    // Is indexed via the addresses of billboards. If an entry is true, then the billboard corresponding to that entry
    // will always be oriented towards the location of the viewer. Otherwise, it will orient towards its last
    std::unordered_map< uintptr_t, bool > mBillboardsOriented;

    uintptr_t billboard_index( const map_tile& billboard ) const { return ( uintptr_t ) reinterpret_cast< const void* >( &billboard ); }

    void fill_orient_map( void );

public:
	bool running = false, mouseShown = true, drawAll = false;
	SDL_Window* window = nullptr;
	SDL_Renderer* renderer = nullptr;
	SDL_GLContext context = nullptr;

    map_tile_list_t billboards;
    map_tile_list_t freeSpace;
    map_tile_list_t walls;

	uint32_t width, height;

	float frameTime, lastTime, startTime;

    std::unique_ptr< map_tile_generator > gen;

	std::unique_ptr< render_pipeline > pipeline;

    plane groundPlane;

    input_client player, spec;

	input_client* camera;

	obb* drawBounds;

    std::unique_ptr< entity > bullet;

    physics_world world;

    view_frustum frustum;

    texture billTexture;

    collision_provider collision;

    application( uint32_t width, uint32_t height );

	~application( void );

    void reset_map( void );

    void toggle_culling( void );

    void tick( void );

    void draw( void );

    void fire_gun( void );

    void update( void );

    map_tile_list_t billboard_list( void ) const { return drawAll? gen->billboards(): billboards; }

    map_tile_list_t wall_list( void ) const { return drawAll? gen->walls(): walls; }

    map_tile_list_t freespace_list( void ) const { return drawAll? gen->freespace(): freeSpace; }

    static application& get_instance( void );

    bool billboard_oriented( const map_tile& billboard ) const
    {
        return mBillboardsOriented.at( billboard_index( billboard ) );
    }

    void billboard_oriented( const map_tile& bb, bool orient )
    {
        mBillboardsOriented[ billboard_index( bb ) ] = orient;
    }
};
