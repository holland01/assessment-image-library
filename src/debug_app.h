#pragma once

#include "application.h"
#include <glm/glm.hpp>

template < typename app_impl_t >
void debug_draw_hud( const application< app_impl_t >& app );

template < typename app_impl_t >
void debug_draw_axes( const application< app_impl_t >& app, const view_data& vp );

template < typename app_impl_t >
void debug_draw_quad( const application< app_impl_t >& app, const glm::mat4& transform, const glm::vec3& color, float alpha = 1.0f );


#include "debug.inl"

