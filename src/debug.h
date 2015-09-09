#pragma once

#include "def.h"
#include <stdint.h>
#include <vector>

struct ray;

using debug_raylist_iter_t = std::vector< ray >::iterator;

bool debug_flag_set( void ); // flag use-case is arbitrary

void debug_set_flag( bool v );

void debug_get_ray( ray& r );

void debug_set_ray( const ray& r );

void debug_raylist_get( uint32_t i, ray& r );

void debug_raylist_push( const ray& r );

debug_raylist_iter_t debug_raylist_begin( void );

debug_raylist_iter_t debug_raylist_end( void );

void debug_raylist_clear( void );

bool debug_raylist_empty( void );
