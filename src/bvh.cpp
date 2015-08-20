#include "bvh.h"
#include "view.h"

//-------------------------------------------------------------------------------------------------------
// quad_hierarchy_t
//
// | II.  | I. |
// | III. | IV. |
//----------------------------------------------------------------------------------------------------

namespace {
    const glm::vec3 PLANE_VEC( 1.0f, 0.0f, 1.0f );
    const glm::vec3 CONST_AXIS_VEC( 0.0f, 1.0f, 0.0f );
    const uint8_t CONST_AXIS = 1;
    const glm::vec3 quadSize( 0.5f, 1.0f, 0.5f );
}

quad_hierarchy_t::quad_hierarchy_t( bounding_box_t bounds, const uint32_t maxDepth, quad_hierarchy_t::entity_list_t entities )
    : root( new node_t( 0, maxDepth, std::move( bounds ) ) )
{
    Update( std::move( entities ) );
}

void quad_hierarchy_t::Update( entity_list_t entities )
{
    root->Update( std::move( entities ), glm::mat4( 1.0f ) );
}

quad_hierarchy_t::node_t::node_t( uint32_t curDepth, const uint32_t maxDepth, bounding_box_t bounds_ )
    : bounds( std::move( bounds_ ) )
{
    if ( curDepth == maxDepth )
    {
        return;
    }

    auto LMakeChild = [ & ]( uint8_t index, const glm::vec3& offset )
    {
        glm::mat4 s( glm::scale( glm::mat4( 1.0f ), quadSize ) );
        glm::mat4 t( glm::translate( glm::mat4( 1.0f ), offset ) );
        glm::mat4 m( t * s );

        children[ index ].reset( new node_t( curDepth + 1, maxDepth, std::move( bounding_box_t( m ) ) ) );
    };

    LMakeChild( 0, quadSize * PLANE_VEC );
    LMakeChild( 1, quadSize * glm::vec3( -1.0f, 0.0f, 1.0f ) );
    LMakeChild( 2, quadSize * glm::vec3( -1.0f , 0.0f, -1.0f ) );
    LMakeChild( 3, quadSize * glm::vec3( 1.0f, 0.0f, -1.0f ) );
}

bool quad_hierarchy_t::node_t::Leaf( void ) const
{
    return !children[ 0 ] && !children[ 1 ] && !children[ 2 ] && !children[ 3 ];
}

void quad_hierarchy_t::node_t::Draw( const pipeline_t& pl, const view_params_t& vp, const glm::mat4& rootTransform ) const
{
    glm::mat4 t( rootTransform * bounds.GetTransform() );

    {
        load_blend_t b( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

        const shader_program_t& singleColor = pl.programs.at( "single_color" );
        const draw_buffer_t& coloredCube = pl.drawBuffers.at( "lined_cube" );

        singleColor.Bind();
        singleColor.LoadVec4( "color", glm::vec4( 0.3f, 0.3f, 0.3f, 1.0f ) );
        singleColor.LoadMat4( "modelToView", vp.transform * t );
        coloredCube.Render( singleColor );
        singleColor.Release();
    }

    for ( const node_t::ptr_t& n: children )
    {
        if ( n )
        {
            n->Draw( pl, vp, t );
        }
    }
}

void quad_hierarchy_t::node_t::Update( quad_hierarchy_t::entity_list_t entities, const glm::mat4& rootTransform )
{
    UNUSEDPARAM( entities );

    if ( Leaf() )
    {
        this->entities = std::move( entities );
        return;
    }

    glm::mat4 t( rootTransform * bounds.GetTransform() );

    std::array< quad_hierarchy_t::entity_list_t, 4 > subregions;

    for( uint32_t i = 0; i < 4; ++i )
    {
        for ( auto e = entities.begin(); e != entities.end(); )
        {
            const entity_t* p = *e;

            if ( p )
            {
                if ( children[ i ]->bounds.Encloses( *( p->bounds ) ) )
                {
                    subregions[ i ].push_back( p );
                    e = entities.erase( e );
                }
                else
                {
                    ++e;
                }
            }
        }
    }

    for ( uint32_t i = 0; i < 4; ++i )
    {
        children[ i ]->Update( std::move( subregions[ i ] ), t );
    }
}
