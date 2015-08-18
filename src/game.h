#pragma once

#include "geom.h"
#include "input.h"
#include "map.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_main.h>
#include <memory>

struct game_t
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

    std::unique_ptr< tile_generator_t > gen;
	std::unique_ptr< pipeline_t > pipeline;

    plane_t groundPlane;

	input_client_t player, spec;
	input_client_t* camera;
    bounding_box_t* drawBounds;

    std::unique_ptr< entity_t > bullet;

	world_t world;

    frustum_t frustum;

    texture_t billTexture;

	game_t( uint32_t width, uint32_t height );
   ~game_t( void );

	void ResetMap( void );
	void ToggleCulling( void );
	void Tick( void );
	void Draw( void );
    void FireGun( void );

	static game_t& GetInstance( void );
};
