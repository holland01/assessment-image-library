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

    // TODO: try only solving for an intersection between two of the rays first.
    // See if you can use algebra to extract the correct scalars
    // over a system of linear equations.
    // (Some linear algebra matrix operations may be useful here)
    glm::vec3 calc_intersection( const ray& r0, const ray& r1, const ray& r2 )
    {
        /*
        const float RANGE = 20.0f;
        const float STEP  = 0.1f;

        glm::vec3 point( -FLT_MIN );

        for ( float s = -RANGE; s <= RANGE; s += STEP )
        {
            for ( float t = -RANGE; t <= RANGE; t += STEP )
            {
                for ( float u = -RANGE; u <= RANGE; u += STEP )
                {
                    glm::vec3 p0( r0.calc_position( s ) );
                    glm::vec3 p1( r1.calc_position( t ) );
                    glm::vec3 p2( r2.calc_position( u ) );

                    if ( p0 == p1 && p1 == p2 )
                    {
                        point = p0;
                        break;
                    }
                }
            }
        }
        */

        return point;
    }
}

void delaunay_test::draw_triangle( imm_draw& draw, const shader_program& prog, const glm::vec3& position )
{
    prog.load_mat4( "modelToView", mCamPtr->view_params().mTransform * glm::translate( glm::mat4( 1.0f ), position ) );

    const float SIZE = 10.0f;

    const std::array< glm::vec3, 3 > verts =
    {{
         glm::vec3( -SIZE, 0.0f, 0.0f ),
         glm::vec3( SIZE, 0.0f, 0.0f ),
         glm::vec3( 0.0f, 0.0f, -SIZE )
    }};

    draw.begin( GL_TRIANGLES );
    draw.vertex( verts[ 0 ], glm::vec4( 1.0f, 0.0f, 0.0f, 1.0f ) );
    draw.vertex( verts[ 1 ], glm::vec4( 0.0f, 1.0f, 0.0f, 1.0f ) );
    draw.vertex( verts[ 2 ], glm::vec4( 0.0f, 0.0f, 1.0f, 1.0f ) );
    draw.end();

    glm::vec3 e1( verts[ 1 ] - verts[ 0 ] );
    glm::vec3 e2( verts[ 2 ] - verts[ 0 ] );
    glm::vec3 e3( verts[ 2 ] - verts[ 1 ] );

    ray bisect1 = make_bisector( verts[ 0 ], e1 );
    ray bisect2 = make_bisector( verts[ 0 ], e2 );
    ray bisect3 = make_bisector( verts[ 1 ], e3 );

    glm::vec3 intersection( calc_intersection( bisect1, bisect2, bisect3 ) );

    __nop();

    bisect1.draw( draw, glm::vec4( 1.0f, 0.0f, 0.0f, 1.0f ) );
    bisect2.draw( draw, glm::vec4( 0.0f, 1.0f, 0.0f, 1.0f ) );
    bisect3.draw( draw, glm::vec4( 1.0f ) );
}


