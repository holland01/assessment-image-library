#pragma once

#include "geom.h"
#include "input.h"
#include "map.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_main.h>
#include <memory>

struct application
{
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

    static application& get_instance( void );
};
