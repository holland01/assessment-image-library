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

	std::vector< const tile_t* > billboards;
	std::vector< const tile_t* > freeSpace;
	std::vector< const tile_t* > walls;

	uint32_t width, height;

	float frameTime, lastTime, startTime;

	std::unique_ptr< generator_t > gen;
	std::unique_ptr< pipeline_t > pipeline;

	plane_t groundPlane;

	input_client_t player, spec;
	input_client_t* camera;
	bounding_box_t* drawBounds;

	world_t world;

	frustum_t frustum;

	game_t( uint32_t width, uint32_t height );
   ~game_t( void );

	void ResetMap( void );
	void ToggleCulling( void );
	void Tick( void );
	void Draw( void );

	static game_t& GetInstance( void );
};
