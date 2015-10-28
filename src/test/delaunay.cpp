#include "delaunay.h"
#include "procedural.h"
#include <random>

namespace {

    const uint32_t FRAME_LIMIT = 10000;

    const uint32_t POINT_COUNT = 1000;

    randomize gRandom( 5.0f, 20.0f );
    randomize gPointRandom( 0.0f, 100.0f );
    randomize gColorRandom( 0.0f, 1.0f );

//    uint32_t gFrameCount = 0;
}

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

    for ( uint32_t i = 0; i < POINT_COUNT; ++i )
    {
        mNodes.push_back( dnode( glm::vec3( gPointRandom(), 0.0f, gPointRandom() ),
                                 glm::vec4( gColorRandom(), gColorRandom(), gColorRandom(), 1.0f ) ) );
    }

    for ( dnode& nodeI: mNodes )
    {
        bool add = true;

        dtri* t = gen_triangle( nodeI.mPoint );

        for ( dnode& nodeJ: mNodes )
        {
            if ( nodeI == nodeJ )
                continue;

            if ( nodeJ.mTriangle )
            {
                float circDist = glm::distance( nodeJ.mTriangle->mCircumCenter, t->mCircumCenter );

                if ( circDist < nodeJ.mTriangle->mCircumRadius + t->mCircumRadius )
                {
                    add = false;
                    break;
                }
            }
        }

        if ( add )
            nodeI.mTriangle.reset( t );
        else
            delete t;
    }
}

void delaunay_test::draw( void )
{
    dt_app_t::draw();

    const shader_program& vcolor = mPipeline->program( "vertex_color" );

    bind_program bind( vcolor );

    imm_draw d( vcolor );

    draw_nodes( d, vcolor );
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

    void draw_circle( imm_draw& draw, const dtri& tri )
    {
        const float twoPiStep = 1.0f / ( glm::two_pi< float >() * tri.mCircumRadius );

        gl_push_float_attrib< glPointSize > pointSize( GL_POINT_SIZE, 1.0f );

        draw.begin( GL_POINTS );
        for ( float theta = 0.0f; theta <= glm::two_pi< float >(); theta += twoPiStep )
        {
            glm::vec3 p( tri.mCircumCenter );

            p.x += glm::cos( theta ) * tri.mCircumRadius;
            p.z += glm::sin( theta ) * tri.mCircumRadius;

            draw.vertex( p, glm::vec4( 1.0f ) );
        }
        draw.end();
    }

    void draw_tri( imm_draw& draw, const dtri& tri )
    {
        const glm::vec4 color( 1.0f );

        draw.begin( GL_TRIANGLES );
        draw.vertex( tri.mVerts[ 0 ], color );
        draw.vertex( tri.mVerts[ 1 ], color );
        draw.vertex( tri.mVerts[ 2 ], color );
        draw.end();
    }
}

dtri* delaunay_test::gen_triangle( const glm::vec3& position )
{
    std::array< glm::vec3, 3 > verts;

    verts[ 0 ] = glm::vec3( -gRandom(), 0.0f, 0.0f );
    verts[ 1 ] = glm::vec3( gRandom(), 0.0f, 0.0f );
    verts[ 2 ] = glm::vec3( 0.0f, 0.0f, -gRandom() );

    glm::vec3 e1( verts[ 1 ] - verts[ 0 ] );
    glm::vec3 e2( verts[ 2 ] - verts[ 0 ] );
    glm::vec3 e3( verts[ 2 ] - verts[ 1 ] );

    ray bisect1 = make_bisector( verts[ 0 ], e1 );
    ray bisect2 = make_bisector( verts[ 0 ], e2 );
    ray bisect3 = make_bisector( verts[ 1 ], e3 );

    glm::vec3 intersection;

    bool result = calc_intersection( bisect1, bisect2, bisect3, intersection );
    assert( result );

    return new dtri( verts, position + intersection, glm::distance( intersection, verts[ 0 ] ) );

    /*

    bisect1.draw( draw, glm::vec4( 1.0f, 0.0f, 0.0f, 1.0f ) );
    bisect2.draw( draw, glm::vec4( 0.0f, 1.0f, 0.0f, 1.0f ) );
    bisect3.draw( draw, glm::vec4( 1.0f ) );
    */
}

void delaunay_test::draw_nodes( imm_draw& draw, const shader_program& prog )
{
    prog.load_mat4( "modelToView", mCamPtr->view_params().mTransform );

    draw.begin( GL_POINTS );
    for ( const dnode& n: mNodes )
    {
        draw.vertex( n.mPoint, n.mColor );
    }
    draw.end();

    for ( const dnode& n: mNodes )
    {
        if ( n.mTriangle )
        {
            prog.load_mat4( "modelToView", mCamPtr->view_params().mTransform * glm::translate( glm::mat4( 1.0f ), n.mPoint ) );
            draw_tri( draw, *( n.mTriangle ) );

            prog.load_mat4( "modelToView", mCamPtr->view_params().mTransform );
            draw_circle( draw, *( n.mTriangle ) );
        }
    }
}

