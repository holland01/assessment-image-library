#include "delaunay.h"
#include "procedural.h"
#include <random>

namespace {

    const uint32_t FRAME_LIMIT = 10000;

    const uint32_t POINT_COUNT = 100;

    randomize gRandom( 5.0f, 20.0f );
    randomize gPointRandom( 0.0f, 100.0f );
    randomize gColorRandom( 0.5f, 1.0f );

    struct tri_gen
    {
        std::vector< const dnode* > mNodes;
        std::vector< std::vector< const dnode* > > mRelatives;

        void sort( const dnode& src, std::vector< const dnode* >& v )
        {
            std::sort( v.begin(), v.end(), [ &src ]( const dnode* a, const dnode* b )
            {
                return glm::distance( src.mPoint, a->mPoint ) < glm::distance( src.mPoint, b->mPoint );
            });
        }

        tri_gen( const dnode& src )
        {
            mNodes.reserve( src.mNeighbors.size() + 1 );
            mNodes.push_back( &src );
            mNodes.insert( mNodes.end(), src.mNeighbors.begin(), src.mNeighbors.end() );

            mRelatives.push_back( src.mNeighbors );

            for ( uint32_t i = 1; i < mNodes.size(); ++i )
            {
                std::vector< const dnode* > neighbors;
                neighbors.resize( mNodes.size() - 1 );

                uint32_t k = 0;

                for ( uint32_t j = 0; i < mNodes.size(); ++j )
                {
                    if ( i == j )
                        continue;

                    neighbors[ k++ ] = mNodes[ j ];
                }

                sort( mNodes[ i ], neighbors );
            }

            // TODO: finish
        }
    };

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

    auto find_min = [ this ]( dnode& n, std::vector< const dnode* >& setA )
    {
        float minDist = FLT_MAX;
        const dnode* minNeighbor = nullptr;

        for ( dnode& m: n.mNeighbors )
        {
            // The only way n could be a neighbor of m is if n is the 1st or 2nd closest
            // point to m, which implies that the triangle corresponding to n and this m is already taken care of - so, move along.
            if ( vector_contains< const dnode* >( setA, &n ) )
                continue;

            float d = glm::distance( n.mPoint, m.mPoint );

            // Let k be the number of times this function is called on n.
            // For n, we want the kth closest neighbor each time.
            if ( !vector_contains< const dnode* >( setA, &m ) && d < minDist )
            {
                minDist = d;
                minNeighbor = &m;
            }
        }

        if ( minNeighbor )
            setA.push_back( minNeighbor );
    };

    for ( dnode& n: mNodes )
    {
        // Find all neighboring nodes
        for ( dnode& m: mNodes )
        {
            if ( n == m )
                continue;

            bool add = true;

            glm::vec3 dirToM( m.mPoint - n.mPoint );

            for ( dnode& k: mNodes )
            {
                if ( k == m || k == n )
                    continue;

                glm::vec3 dirToK( k.mPoint - n.mPoint );

                // If we have a point which is closer and in the same line of sight, we need to bail
                if ( glm::distance( k.mPoint, n.mPoint ) < glm::distance( m.mPoint, n.mPoint )
                     && glm::dot( dirToM, dirToK ) >= 0.999f ) // if > 0.8, they may as well be parallel
                {
                    add = false;
                    break;
                }
            }

            if ( add )
                n.mNeighbors.push_back( &m );
        }

        if ( n.mNeighbors.size() < 2 )
        {
            n.mNeighbors.clear();
            continue;
        }

        // Partition nodes into triangles


        std::vector< const dnode* > allp;
        allp.push_back( &n );
        allp.insert( allp.end(), n.mNeighbors.begin(), n.mNeighbors.end() );

        for ( uint32_t i = 0; i < allp.size(); i += 3 )
        {
            dtri t = gen_triangle(  )
        }
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

dtri delaunay_test::gen_triangle( const dnode* a, const dnode* b, const dnode* c )
{
    std::array< dtri::vertex_type, 3 > verts;

    verts[ 0 ] = a;
    verts[ 1 ] = b;
    verts[ 2 ] = c;

    glm::vec3 e1( verts[ 1 ].mPoint - verts[ 0 ].mPoint );
    glm::vec3 e2( verts[ 2 ].mPoint - verts[ 0 ].mPoint );
    glm::vec3 e3( verts[ 2 ].mPoint - verts[ 1 ].mPoint );

    ray bisect1 = make_bisector( verts[ 0 ].mPoint, e1 );
    ray bisect2 = make_bisector( verts[ 0 ].mPoint, e2 );
    ray bisect3 = make_bisector( verts[ 1 ].mPoint, e3 );

    glm::vec3 intersection;

    bool result = calc_intersection( bisect1, bisect2, bisect3, intersection );
    assert( result );

    return dtri( verts, position + intersection, glm::distance( intersection, verts[ 0 ] ) );
}

void delaunay_test::draw_nodes( imm_draw& draw, const shader_program& prog )
{
    prog.load_mat4( "modelToView", mCamPtr->view_params().mTransform );

    draw.begin( GL_POINTS );
    for ( const dnode& n: mNodes )
        draw.vertex( n.mPoint, n.mColor );
    draw.end();

    for ( const dnode& n: mNodes )
    {
        if ( n.mTriangles )
        {
            prog.load_mat4( "modelToView", mCamPtr->view_params().mTransform * glm::translate( glm::mat4( 1.0f ), n.mPoint ) );
            draw_tri( draw, *( n.mTriangles ) );

            prog.load_mat4( "modelToView", mCamPtr->view_params().mTransform );
            draw_circle( draw, *( n.mTriangles ) );
        }
        else
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
}

