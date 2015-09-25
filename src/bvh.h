#pragma once

#include "def.h"
#include "entity.h"
#include "geom/geom.h"
#include <vector>

//-------------------------------------------------------------------------------------------------------
// quad_hierarchy_t
//
// | II.  | I. |
// | III. | IV. |
//-------------------------------------------------------------------------------------------------------

struct quad_hierarchy
{
    using entity_list_t = std::vector< const entity* >;
    using ptr_t = std::unique_ptr< quad_hierarchy >;

    static const uint8_t NODE_COUNT = 4;

    struct node
    {
        using ptr_t = std::unique_ptr< node >;

        obb mLocalBounds;

        obb mWorldBounds;

        bool mShouldDestroy, mChildrenDestroyed;

        // Only leaf nodes will have entities
        entity_list_t mEntities;

        std::array< ptr_t, NODE_COUNT > mChildren;

		node( uint32_t curDepth, const uint32_t maxDepth, obb mLocalBounds, const transform_data& parentAxes = transform_data() );

        void make_child( const uint32_t curDepth, const uint32_t maxDepth, const uint8_t index, const glm::vec3& offset );

        void draw( const render_pipeline& pl, const view_data& vp, const glm::mat4& rootTransform = glm::mat4( 1.0f ) ) const;

        void update( entity_list_t mEntities, const glm::mat4& rootTransform = glm::mat4( 1.0f ) );

		bool destroy( void ) const { return mShouldDestroy; }

        bool world_transform_valid( const glm::mat4& t ) const;

        bool leaf( void ) const;
    };

    node::ptr_t mRoot;

    quad_hierarchy( obb bounds, const uint32_t maxDepth, entity_list_t entities = entity_list_t() );

    void update( entity_list_t entities );
};

INLINE bool quad_hierarchy::node::world_transform_valid( const glm::mat4& t ) const
{
	glm::mat4 m( mWorldBounds.world_transform() * t );
    float d = glm::determinant( glm::mat3( m ) );
    return d >= 1.0f;
}
