#pragma once

#include "application.h"
#include <glm/glm.hpp>

template < typename app_impl_t >
void debug_draw_hud( const application< app_impl_t >& app );

template < typename app_impl_t >
void debug_draw_axes( const application< app_impl_t >& app, const view_data& vp );

template < typename app_impl_t >
void debug_draw_bounds( const application< app_impl_t >& app, const obb& bounds, const glm::vec3& color, float alpha = 1.0f );

template < typename app_impl_t >
void debug_draw_billboard_bounds( const application< app_impl_t >& app,
                                  const view_data& vp,
                                  const obb& billboardBounds );

template < typename app_impl_t >
void debug_draw_debug_ray(  application< app_impl_t >& app,
                            imm_draw& d,
                            const obb& bounds,
                            const ray& debugRay );

template < typename app_impl_t >
void debug_draw_debug_ray_list( application< app_impl_t >& app, const obb& bounds );

template < typename app_impl_t >
void debug_draw_quad( const application< app_impl_t >& app, const glm::mat4& transform, const glm::vec3& color, float alpha = 1.0f );


#include "debug.inl"

