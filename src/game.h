#pragma once

#include "application.h"
#include "physics_world.h"
#include "map.h"

struct game;

using game_app_t = application< game >;

struct game : public application< game >
{
private:
    // Is indexed via the addresses of billboards. If an entry is true, then the billboard corresponding to that entry
    // will always be oriented towards the location of the viewer. Otherwise, it will orient towards its last
    std::unordered_map< uintptr_t, bool > mBillboardsOriented;

    void fill_orient_map( void );

    mutable physics_world mWorld;

public:
    std::unique_ptr< entity > bullet;

    texture billTexture;

    game( uint32_t width, uint32_t height );

    void frame( void ) override;

    void update( void ) override;

    void handle_event( const SDL_Event& e ) override;

    void draw( void ) override;

    void fire_gun( void );
};

