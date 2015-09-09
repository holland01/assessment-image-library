#pragma once

#include "application.h"

using physics_app_t = application< physics_simulation >;

struct physics_simulation : public application< physics_simulation >
{
public:
    physics_simulation( void );

    ~physics_simulation( void );

    void frame( void ) override;
};
