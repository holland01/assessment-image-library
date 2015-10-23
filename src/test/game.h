#pragma once

#include "application.h"
#include "physics/physics_world.h"
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
    texture mBillTexture;

    game( uint32_t mWidth, uint32_t mHeight );

    void frame( void ) override;

    void update( void ) override;

    void handle_event( const SDL_Event& e ) override;

    void draw( void ) override;
};

