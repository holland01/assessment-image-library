#include "delaunay.h"


delaunay_test::delaunay_test( uint32_t width, uint32_t height )
    : dt_app_t( width, height )
{
    mSpec.position( glm::vec3( 0.0f, 10.0f, 0.0f ) );
}

void delaunay_test::draw( void )
{
    dt_app_t::draw();
}

void delaunay_test::frame( void )
{
    draw();
}


