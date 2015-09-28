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

    glm::vec3 quadSize( 0.5f, 1.0f, 0.5f );
}

quad_hierarchy::quad_hierarchy( obb bounds, const uint32_t maxDepth, quad_hierarchy::entity_list_t entities )
    : mRoot( new node( 0, maxDepth, std::move( bounds ) ) )
{
    update( std::move( entities ) );
}

void quad_hierarchy::update( entity_list_t entities )
{
    mRoot->update( std::move( entities ), glm::mat4( 1.0f ) );
}

quad_hierarchy::node::node( uint32_t curDepth, const uint32_t maxDepth, obb bounds_, const transform_data& parentAxes )
    : mLocalBounds( std::move( bounds_ ) ),
	  mWorldBounds( parentAxes ),
      mShouldDestroy( false )
{
	float d = glm::determinant( mWorldBounds.axes() );

    mShouldDestroy = d < 16.0f;

    if ( mShouldDestroy )
    {
        return;
    }

    make_child( curDepth, maxDepth, 0, quadSize * PLANE_VEC );
    make_child( curDepth, maxDepth, 1, quadSize * glm::vec3( -1.0f, 0.0f, 1.0f ) );
    make_child( curDepth, maxDepth, 2, quadSize * glm::vec3( -1.0f , 0.0f, -1.0f ) );
    make_child( curDepth, maxDepth, 3, quadSize * glm::vec3( 1.0f, 0.0f, -1.0f ) );

    for ( ptr_t& n: mChildren )
    {
        if ( n && n->destroy() )
        {
            n.release();
        }
    }
}

void quad_hierarchy::node::make_child( const uint32_t curDepth,
                                       const uint32_t maxDepth,
                                       const uint8_t index,
                                       const glm::vec3& offset )
{
	transform_data localTrans( glm::mat3( 1.0f ), quadSize, offset );
	obb localBounds( localTrans );

	mChildren[ index ].reset( new node( curDepth + 1,
										maxDepth,
										std::move( localBounds ),
										localTrans.transform_to( mWorldBounds.trans_data() ) ) );
}

bool quad_hierarchy::node::leaf( void ) const
{
    return !mChildren[ 0 ] && !mChildren[ 1 ] && !mChildren[ 2 ] && !mChildren[ 3 ];
}

void quad_hierarchy::node::draw( const render_pipeline& pl, const view_data& vp, const glm::mat4& rootTransform ) const
{
	glm::mat4 t( rootTransform * mLocalBounds.world_transform() );

    if ( leaf() )
    {
        set_blend_mode b( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

        const shader_program& singleColor = pl.programs().at( "single_color" );
        const draw_buffer& linedCube = pl.draw_buffers().at( "lined_cube" );

        glm::vec4 color( rand_color( 0.5f, 1.0f, 0.5f ) );

        singleColor.bind();
        singleColor.load_vec4( "color", color );
        singleColor.load_mat4( "modelToView", vp.mTransform * t );
        linedCube.render( singleColor );

        const draw_buffer& coloredCube = pl.draw_buffers().at( "colored_cube" );

        for ( const entity* e: mEntities )
        {
            const obb& bounds = *ENTITY_PTR_GET_BOX( e, ENTITY_BOUNDS_AREA_EVAL );

            singleColor.load_vec4( "color", color );
			singleColor.load_mat4( "modelToView",  vp.mTransform * bounds.world_transform() );
            coloredCube.render( singleColor );
        }

        singleColor.release();
    }

    for ( const node::ptr_t& n: mChildren )
    {
        if ( n )
        {
            n->draw( pl, vp, t );
        }
    }
}

void quad_hierarchy::node::update( quad_hierarchy::entity_list_t entities, const glm::mat4& rootTransform )
{
    if ( leaf() )
    {
        this->mEntities = std::move( entities );
        return;
    }

	glm::mat4 t( rootTransform * mLocalBounds.world_transform() );

    std::array< quad_hierarchy::entity_list_t, 4 > subregions;

    for( uint32_t i = 0; i < 4; ++i )
    {
		transform_data tt( mChildren[ i ]
						   ->mLocalBounds.trans_data().transform_to( mWorldBounds.trans_data() ) );

		const obb childBounds( std::move( obb( tt ) ) );

        for ( auto e = entities.begin(); e != entities.end(); )
        {
            const entity* p = *e;

            if ( p )
            {
                const obb* pbox = ENTITY_PTR_GET_BOX( p, ENTITY_BOUNDS_AREA_EVAL );

                if ( childBounds.encloses( *pbox ) )
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
        mChildren[ i ]->update( std::move( subregions[ i ] ), t );
    }
}
