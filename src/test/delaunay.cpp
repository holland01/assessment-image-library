#include "delaunay.h"
#include "procedural.h"
#include <random>

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

    draw_triangle( d, vcolor, glm::vec3( 50.0f, 0.0f, 50.0f ) );
}

void delaunay_test::frame( void )
{
    update();
    draw();

    mCamPtr->print_origin();
}

namespace {
    ray make_bisector( const glm::vec3& startVertex, const glm::vec3& edge )
    {
        glm::vec3 p0( startVertex + edge * 0.5f );
        glm::vec3 dir( glm::cross( edge, G_DIR_UP ) );

        ray bisect( p0, glm::normalize( dir ), glm::length( dir ) );

        return bisect;
    }

    bool calc_intersection( const ray& r0, const ray& r1, const ray& r2, glm::vec3& point )
    {
        point = glm::vec3( FLT_MAX );

        float r0T, r1T, r2T;

        if ( !r0.intersects( r1, r0T, r1T ) )
            return false;

        if ( !r0.intersects( r2, r0T, r2T ) )
            return false;

        point = glm::vec3( r0.calc_position( r0T ) );

        return true;
    }
}

namespace {

    const uint32_t FRAME_LIMIT = 10000;

    randomize gRandom( 5.0f, 20.0f );

    uint32_t gFrameCount = 0;

    std::array< glm::vec3, 3 > gVerts;
}

void delaunay_test::draw_triangle( imm_draw& draw, const shader_program& prog, const glm::vec3& position )
{
    prog.load_mat4( "modelToView", mCamPtr->view_params().mTransform * glm::translate( glm::mat4( 1.0f ), position ) );

    if ( gFrameCount++ % FRAME_LIMIT == 0 )
    {
        gVerts[ 0 ] = glm::vec3( -gRandom(), 0.0f, 0.0f );
        gVerts[ 1 ] = glm::vec3( gRandom(), 0.0f, 0.0f );
        gVerts[ 2 ] = glm::vec3( 0.0f, 0.0f, -gRandom() );
    }

    draw.begin( GL_TRIANGLES );
    draw.vertex( gVerts[ 0 ], glm::vec4( 1.0f, 0.0f, 0.0f, 1.0f ) );
    draw.vertex( gVerts[ 1 ], glm::vec4( 0.0f, 1.0f, 0.0f, 1.0f ) );
    draw.vertex( gVerts[ 2 ], glm::vec4( 0.0f, 0.0f, 1.0f, 1.0f ) );
    draw.end();

    glm::vec3 e1( gVerts[ 1 ] - gVerts[ 0 ] );
    glm::vec3 e2( gVerts[ 2 ] - gVerts[ 0 ] );
    glm::vec3 e3( gVerts[ 2 ] - gVerts[ 1 ] );

    ray bisect1 = make_bisector( gVerts[ 0 ], e1 );
    ray bisect2 = make_bisector( gVerts[ 0 ], e2 );
    ray bisect3 = make_bisector( gVerts[ 1 ], e3 );

    glm::vec3 intersection;

    if ( calc_intersection( bisect1, bisect2, bisect3, intersection ) )
    {
        draw.begin( GL_POINTS );
        draw.vertex( intersection, glm::vec4( 1.0f, 1.0f, 0.0f, 1.0f ) );
        draw.end();

        float radius = glm::distance( gVerts[ 0 ], intersection );

        const float twoPiStep = 1.0f / ( glm::two_pi< float >() * radius );

        gl_push_float_attrib< glPointSize > pointSize( GL_POINT_SIZE, 1.0f );

        draw.begin( GL_POINTS );
        for ( float theta = 0.0f; theta <= glm::two_pi< float >(); theta += twoPiStep )
        {
            glm::vec3 p( intersection );

            p.x += glm::cos( theta ) * radius;
            p.z += glm::sin( theta ) * radius;

            draw.vertex( p, glm::vec4( 1.0f ) );
        }
        draw.end();
    }

    bisect1.draw( draw, glm::vec4( 1.0f, 0.0f, 0.0f, 1.0f ) );
    bisect2.draw( draw, glm::vec4( 0.0f, 1.0f, 0.0f, 1.0f ) );
    bisect3.draw( draw, glm::vec4( 1.0f ) );
}


