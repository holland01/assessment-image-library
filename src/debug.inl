
template < typename app_impl_t >
INLINE void debug_draw_hud( const application< app_impl_t >& app )
{
    // Clear depth buffer so the reticule renders over anything else it tests
    // against by default
    GL_CHECK( glClear( GL_DEPTH_BUFFER_BIT ) );

    const shader_program& ssSingleColor = app.pipeline().programs().at( "single_color_ss" );

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
    const shader_program& singleColor = app.pipeline().programs().at( "single_color" );

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
INLINE void debug_draw_quad( const application< app_impl_t >& app, const glm::mat4& transform, const glm::vec3& color, float alpha )
{
    const shader_program& singleColor = app.pipeline().programs().at( "single_color" );
    const draw_buffer& billboardBuffer = app.pipeline().draw_buffers().at( "billboard" );
    const view_data& vp = app.camera->view_params();

    singleColor.bind();
    singleColor.load_vec4( "color", glm::vec4( color, alpha ) );
    singleColor.load_mat4( "modelToView", vp.mTransform * transform );
    billboardBuffer.render( singleColor );
    singleColor.release();
}
