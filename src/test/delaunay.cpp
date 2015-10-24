#include "delaunay.h"


delaunay_test::delaunay_test( uint32_t width, uint32_t height )
    : dt_app_t( width, height )
{
    mCamPtr->position( glm::vec3( 49.0f, 10.0f, 49.0f ) );

    glm::vec3 point( 0.0f, 0.0f, 0.0f );
    glm::vec3 forward( glm::normalize( point - mCamPtr->position() ) );
    glm::vec3 right( glm::normalize( glm::cross( forward, G_DIR_UP ) ) );
    glm::vec3 up( glm::normalize( glm::cross( forward, right ) ) );
    right = glm::normalize( glm::cross( forward, up ) );

    glm::mat3 orient( 1.0f );
    orient[ 0 ] = -right;
    orient[ 1 ] = -up;
    orient[ 2 ] = -forward;

    mCamPtr->orientation( glm::inverse( orient ) );
    mCamPtr->flags( input_client::flags::lock_orientation );
}

void delaunay_test::draw( void )
{
    dt_app_t::draw();
}

void delaunay_test::frame( void )
{
    update();
    draw();

    mCamPtr->print_origin();
}


