#pragma once

#include "application.h"

struct physics_simulation : public application< physics_simulation >
{
public:
    physics_simulation( uint32_t width, uint32_t height );

    void frame( void ) override;

    void draw( void ) override;

    void fill_entities( std::vector< entity* >& entities ) const override;

    void handle_event( const SDL_Event& e ) override;
};

using physics_app_t = application< physics_simulation >;
