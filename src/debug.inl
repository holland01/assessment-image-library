
template < typename app_impl_t >
INLINE void debug_draw_hud( const application< app_impl_t >& app )
{
    // Clear depth buffer so the reticule renders over anything else it tests
    // against by default
    GL_CHECK( glClear( GL_DEPTH_BUFFER_BIT ) );

    const shader_program& ssSingleColor = app.pipeline->programs().at( "single_color_ss" );

    ssSingleColor.bind();
    ssSingleColor.load_vec4( "color", glm::vec4( 1.0f ) );

    imm_draw drawer( ssSingleColor );

    // Draw reticule
    drawer.begin( GL_POINTS );
    drawer.vertex( glm::vec3( 0.0f ) );
    drawer.end();

    ssSingleColor.release();
}

template < typename app_impl_t >
INLINE void debug_draw_axes( const application< app_impl_t >& app, const view_data& vp )
{
    const shader_program& singleColor = app.pipeline->programs().at( "single_color" );

    singleColor.bind();
    singleColor.load_mat4( "modelToView", vp.mTransform * glm::translate( glm::mat4( 1.0f ), glm::vec3( 0.0f, 1.0f, 0.0f ) ) );
    singleColor.load_vec4( "color", glm::vec4( 1.0f ) );

    imm_draw drawer( singleColor );

    const float AXIS_SIZE = 100.0f;

    // Draw coordinate space axes
    drawer.begin( GL_LINES );
    drawer.vertex( glm::vec3( 0.0f ) );
    drawer.vertex( glm::vec3( 0.0f, 0.0f, AXIS_SIZE ) );
    drawer.vertex( glm::vec3( 0.0f ) );
    drawer.vertex( glm::vec3( 0.0f, AXIS_SIZE, 0.0f ) );
    drawer.vertex( glm::vec3( 0.0f ) );
    drawer.vertex( glm::vec3( AXIS_SIZE, 0.0f, 0.0f ) );
    drawer.end();

    singleColor.release();
}

template < typename app_impl_t >
INLINE void debug_draw_bounds( const application< app_impl_t >& app, const obb& bounds, const glm::vec3& color, float alpha )
{
    const shader_program& singleColor = app.pipeline->programs().at( "single_color" );
    const draw_buffer& coloredCube = app.pipeline->draw_buffers().at( "colored_cube" );
    const view_data& vp = app.camera->view_params();

    singleColor.load_vec4( "color", glm::vec4( color, alpha ) );
    singleColor.load_mat4( "modelToView",  vp.mTransform * bounds.axes() );
    coloredCube.render( singleColor );
}

template < typename app_impl_t >
INLINE void debug_draw_billboard_bounds( const application< app_impl_t >& app,
                                  const view_data& vp,
                                  const obb& billboardBounds )
{
    const shader_program& singleColor = app.pipeline->programs().at( "single_color" );

    singleColor.bind();
    debug_draw_bounds( app, billboardBounds, glm::vec3( 0.5f ), 0.5f );

    singleColor.load_vec4( "color", glm::vec4( 1.0f, 0.0f, 0.0f, 1.0f ) );
    singleColor.load_mat4( "modelToView", vp.mTransform * billboardBounds.axes() );

    imm_draw drawer( singleColor );

    drawer.begin( GL_LINES );
    drawer.vertex( glm::vec3( 0.0f ) );
    drawer.vertex( glm::vec3( 0.0f, 0.0f, 3.0f ) );
    drawer.end();
}

template < typename app_impl_t >
INLINE void debug_draw_debug_ray( application< app_impl_t >& app,
                            imm_draw& d,
                            const obb& bounds,
                            const ray& debugRay )
{
    const shader_program& singleColor = app.pipeline->programs().at( "single_color" );

    singleColor.load_mat4( "modelToView", app.camera->view_params().mTransform );

    singleColor.load_vec4( "color", glm::vec4( 1.0f ) );

    d.begin( GL_LINES );
    d.vertex( debugRay.p );
    d.vertex( debugRay.calc_position() );
    d.end();

    singleColor.load_vec4( "color", glm::vec4( 1.0f, 0.0f, 0.0f, 1.0f ) );

    d.begin( GL_POINTS );
    d.vertex( debugRay.p );
    d.end();

    singleColor.load_vec4( "color", glm::vec4( 0.0f, 0.0f, 1.0f, 1.0f ) );

    d.begin( GL_POINTS );
    d.vertex( debugRay.calc_position() );
    d.end();

    obb::maxmin_pair3D_t mm = bounds.maxmin( false );

    singleColor.load_vec4( "color", glm::vec4( 1.0f, 0.0f, 1.0f, 1.0f ) );

    d.begin( GL_POINTS );
    d.vertex( mm.max );
    d.end();

    singleColor.load_vec4( "color", glm::vec4( 0.0f, 1.0f, 1.0f, 1.0f ) );

    d.begin( GL_POINTS );
    d.vertex( mm.min );
    d.end();
}

template < typename app_impl_t >
INLINE void debug_draw_debug_ray_list( application< app_impl_t >& app, const obb& bounds )
{
    const shader_program& singleColor = app.pipeline->programs().at( "single_color" );
    imm_draw d( singleColor );

    for ( auto i = debug_raylist_begin(); i != debug_raylist_end(); ++i )
    {
        debug_draw_debug_ray( app, d, bounds, *i );
    }
}

template < typename app_impl_t >
INLINE void debug_draw_quad( const application< app_impl_t >& app, const glm::mat4& transform, const glm::vec3& color, float alpha )
{
    const shader_program& singleColor = app.pipeline->programs().at( "single_color" );
    const draw_buffer& billboardBuffer = app.pipeline->draw_buffers().at( "billboard" );
    const view_data& vp = app.camera->view_params();

    singleColor.bind();
    singleColor.load_vec4( "color", glm::vec4( color, alpha ) );
    singleColor.load_mat4( "modelToView", vp.mTransform * transform );
    billboardBuffer.render( singleColor );
    singleColor.release();
}
