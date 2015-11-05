#include "delaunay.h"
#include "procedural.h"
#include <random>

namespace {

    const uint32_t FRAME_LIMIT = 10000;

    const uint32_t POINT_COUNT = 100;

    const float POINT_STRIDE = 1.0f;

    randomize gRandom( 5.0f, 20.0f );
    randomize gPointRandom( 0.0f, 100.0f );
    randomize gColorRandom( 0.3f, 0.7f );

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

    dtri gen_triangle( const dnode* a, const dnode* b, const dnode* c )
    {
        std::array< dtri::vertex_type, 3 > verts;

        verts[ 0 ] = a;
        verts[ 1 ] = b;
        verts[ 2 ] = c;

        glm::vec3 e1( verts[ 1 ]->mPoint - verts[ 0 ]->mPoint );
        glm::vec3 e2( verts[ 2 ]->mPoint - verts[ 0 ]->mPoint );
        glm::vec3 e3( verts[ 2 ]->mPoint - verts[ 1 ]->mPoint );

        ray bisect1 = make_bisector( verts[ 0 ]->mPoint, e1 );
        ray bisect2 = make_bisector( verts[ 0 ]->mPoint, e2 );
        ray bisect3 = make_bisector( verts[ 1 ]->mPoint, e3 );

        glm::vec3 intersection;

        bool result = calc_intersection( bisect1, bisect2, bisect3, intersection );
        assert( result );

        return dtri( verts, intersection, glm::distance( intersection, verts[ 0 ]->mPoint ) );
    }
}

delaunay_test::delaunay_test( uint32_t width, uint32_t height )
    : dt_app_t( width, height )
{
    mCamPtr->position( glm::vec3( 50.0f, 200.0f, 50.0f ) );
    //mCamPtr->move_step( 10.0f );

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

    for ( uint32_t i = 0; i < POINT_COUNT; ++i )
    {
        mNodes.push_back( dnode( i,
                                 glm::vec3( gPointRandom() * POINT_STRIDE, 0.0f, gPointRandom() * POINT_STRIDE ),
                                 glm::vec4( gColorRandom(), gColorRandom(), gColorRandom(), 1.0f ) ) );
    }

    const float ONE_OVER_TWO_PI = glm::one_over_two_pi< float >() * 0.1f;

    for ( float theta = 0.0f; theta <= glm::two_pi< float >(); theta += ONE_OVER_TWO_PI )
    {
        glm::vec3 r( glm::cos( theta ), 0.0f, glm::sin( theta ) );
        ray r0( glm::vec3( 50.0f, 0.0f, 50.0f ), r );

        float maxDist = FLT_MIN;
        dnode* pN = nullptr;

        for ( dnode& n: mNodes )
        {
            glm::vec3 dx( n.mPoint - r0.mOrigin );

            float L = glm::length( dx );

            if ( glm::dot( glm::normalize( dx ), r ) >= 0.99f && L > maxDist )
            {
                maxDist = L;
                pN = &n;
            }
        }

        if ( pN )
        {
            pN->mInternal = false;
            mConvexHull.push_back( pN );
        }
    }

    for ( const dnode& n: mNodes )
        if ( n.mInternal )
            mInternal.push_back( &n );

    auto find_closest_point = [ this ]( const dnode& k, const float low ) -> std::tuple< float, const dnode* >
    {
        float minDist = FLT_MAX;
        dnode* p = nullptr;

        uint32_t x = 0;

        for ( uint32_t i = 0; i < mNodes.size(); ++i )
        {
            dnode& n = mNodes[ i ];

            if ( n.mInternal )
            {
                if ( ( x++ % 3 ) == 0 )
                    continue;
            }


            float dist = glm::distance( k.mPoint, n.mPoint );

            if ( !n.mTriVertex && low < dist && dist < minDist )
            {
                p = &n;
                minDist = dist;
            }
        }

        if ( p )
            p->mTriVertex = true;

        return std::tuple< float, const dnode* >( minDist, p );
    };

    const uint32_t internalCount = mInternal.size() / 3;

    for ( uint32_t i = 0, k = 0; k < internalCount; ( i += 3, k += 1 ) )
    {
        mNodes[ mInternal[ i ]->mIndex ].mColor = glm::vec4( 1.0f );
        mNodes[ mInternal[ i ]->mIndex ].mTriCenter = true;

        auto p0 = find_closest_point( *( mInternal[ i ] ), 0.0f );
        auto p1 = find_closest_point( *( mInternal[ i ] ), std::get< 0 >( p0 ) );
        auto p2 = find_closest_point( *( mInternal[ i ] ), std::get< 0 >( p1 ) );

        std::array< const dnode*, 3 > v = {{ std::get< 1 >( p0 ),
                                             std::get< 1 >( p1 ),
                                             std::get< 1 >( p2 ) }};
        dinternal_node in;
        in.mInner = mInternal[ i ];
        in.mOuter = std::move( v );

        mTriGraph.push_back( std::move( in ) );
        mTriangles.push_back( gen_triangle( in.mOuter[ 0 ], in.mOuter[ 1 ], in.mOuter[ 2 ] ) );
    }
}

void delaunay_test::draw( void )
{
    dt_app_t::draw();

    const shader_program& vcolor = mPipeline->program( "vertex_color" );

    bind_program bind( vcolor );

    imm_draw d( vcolor );

    draw_hull( d, vcolor );
    draw_nodes( d, vcolor );
}

void delaunay_test::frame( void )
{
    update();
    draw();

    mCamPtr->print_origin();
}

namespace {

    void draw_circle( imm_draw& draw, const dtri& tri )
    {
        const float twoPiStep = 1.0f / ( glm::two_pi< float >() * tri.mCircumRadius );

        gl_push_float_attrib< GL_POINT_SIZE, glPointSize > pointSize( 1.0f );

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
        const glm::vec4 color( 1.0f, 0.0f, 1.0f, 1.0f );

        draw.begin( GL_TRIANGLES );
        draw.vertex( tri.mVerts[ 0 ]->mPoint, color );
        draw.vertex( tri.mVerts[ 1 ]->mPoint, color );
        draw.vertex( tri.mVerts[ 2 ]->mPoint, color );
        draw.end();
    }
}

void delaunay_test::draw_hull( imm_draw& draw, const shader_program& prog )
{
    prog.load_mat4( "modelToView", mCamPtr->view_params().mTransform );

    glm::vec4 color( 1.0f );

    draw.begin( GL_LINES );
    for ( uint32_t i = 0; i < mConvexHull.size(); ++i )
    {
        draw.vertex( mConvexHull[ i ]->mPoint, color );
        draw.vertex( mConvexHull[ ( i + 1 ) % mConvexHull.size() ]->mPoint, color );
    }
    draw.end();
}

void delaunay_test::draw_nodes( imm_draw& draw, const shader_program& prog )
{
    prog.load_mat4( "modelToView", mCamPtr->view_params().mTransform );

    draw.begin( GL_POINTS );
    for ( const dnode& n: mNodes )
        draw.vertex( n.mPoint, n.mColor );
    draw.end();


    gl_push_polygon_mode s( GL_LINE );
    for ( const dtri& t: mTriangles )
    {
        prog.load_mat4( "modelToView", mCamPtr->view_params().mTransform );
        draw_tri( draw, t );

        //prog.load_mat4( "modelToView", mCamPtr->view_params().mTransform );
        //draw_circle( draw, t );
    }

    /*
    for ( const dnode& n: mNodes )
    {
        if ( !n.mTriangles.empty() )
        {
        }
        //else

        {
            prog.load_mat4( "modelToView", mCamPtr->view_params().mTransform );

            draw.begin( GL_LINES );

            std::vector< const dnode* > all( n.mNeighbors.begin(), n.mNeighbors.end() );
            all.push_back( &n );

            for ( uint32_t i = 0; i < all.size(); ++i )
            {
                draw.vertex( all[ i ]->mPoint, n.mColor );
                draw.vertex( all[ ( i + 1 ) % all.size() ]->mPoint, n.mColor );
            }

            draw.end();
        }

    }
    */
}

