#include "delaunay.h"


delaunay_test::delaunay_test( uint32_t width, uint32_t height )
    : dt_app_t( width, height )
{
    mCamPtr->position( glm::vec3( 50.0f, 200.0f, 50.0f ) );

    glm::vec3 point( 50.0f, 0.0f, 50.0f );
    glm::vec3 forward( glm::normalize( point - mCamPtr->position() ) );
    glm::vec3 up( glm::normalize( glm::cross( forward, G_DIR_RIGHT ) ) );
    glm::vec3 right( glm::normalize( glm::cross( forward, up ) ) );

    glm::mat3 orient( 1.0f );
    orient[ 0 ] = -right;
    orient[ 1 ] = -up;
    orient[ 2 ] = -forward;

    mCamPtr->orientation( glm::inverse( orient ) );
    mCamPtr->flags( input_client::flags::lock_orientation );

    set_screen_dim_ortho( 0.1f );
    load_clip_transform();
}

void delaunay_test::draw( void )
{
    dt_app_t::draw();

    const shader_program& vcolor = mPipeline->program( "vertex_color" );

    bind_program bind( vcolor );

    imm_draw d( vcolor );

    vcolor.load_mat4( "modelToView", mCamPtr->view_params().mTransform * glm::translate( glm::mat4( 1.0f ), glm::vec3( 50.0f, 0.0f, 50.0f ) ) );

    const float SIZE = 10.0f;

    d.begin( GL_TRIANGLES );
    d.vertex( glm::vec3( -SIZE, 0.0f, 0.0f ), glm::vec4( 1.0f, 0.0f, 0.0f, 1.0f ) );
    d.vertex( glm::vec3( SIZE, 0.0f, 0.0f ), glm::vec4( 0.0f, 1.0f, 0.0f, 1.0f ) );
    d.vertex( glm::vec3( 0.0f, 0.0f, -SIZE ), glm::vec4( 0.0f, 0.0f, 1.0f, 1.0f ) );
    d.end();
}

void delaunay_test::frame( void )
{
    update();
    draw();

    mCamPtr->print_origin();
}


