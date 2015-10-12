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

    uintptr_t billboard_index( const map_tile& billboard ) const { return ( uintptr_t ) reinterpret_cast< const void* >( &billboard ); }

    void fill_orient_map( void );

    mutable physics_world mWorld;

public:
    std::unique_ptr< entity > bullet;

    texture billTexture;

    map_tile_list_t billboards;

    map_tile_list_t freeSpace;

    map_tile_list_t walls;

    std::unique_ptr< map_tile_generator > gen;

    game( uint32_t width, uint32_t height );

    void frame( void ) override;

    void update( void ) override;

    void handle_event( const SDL_Event& e ) override;

    void draw( void ) override;

    void fill_entities( std::vector< entity* >& list ) const override;

    void clear_entities( std::vector< entity* >& list ) override;

    const map_tile* reset_map( void ); // returns a starting tile with which the player can use

    void fire_gun( void );

    map_tile_list_t billboard_list( void ) const { return drawAll? gen->billboards(): billboards; }

    map_tile_list_t wall_list( void ) const { return drawAll? gen->walls(): walls; }

    map_tile_list_t freespace_list( void ) const { return drawAll? gen->freespace(): freeSpace; }

    bool billboard_oriented( const map_tile& billboard ) const { return mBillboardsOriented.at( billboard_index( billboard ) ); }

    void billboard_oriented( const map_tile& bb, bool orient );
};

INLINE void game::billboard_oriented( const map_tile& bb, bool orient )
{
    mBillboardsOriented[ billboard_index( bb ) ] = orient;
}

