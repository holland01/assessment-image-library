#pragma once

#include "def.h"
#include "entity.h"
#include "geom.h"
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

        bool mShouldDestroy;

        // Only leaf nodes will have entities
        entity_list_t mEntities;

        std::array< ptr_t, NODE_COUNT > mChildren;

        node( uint32_t curDepth, const uint32_t maxDepth, obb mLocalBounds, const glm::mat4& parentAxes = glm::mat4( 1.0f ) );

        void make_child( const uint32_t curDepth, const uint32_t maxDepth, uint8_t index, const glm::vec3& offset );

        void draw( const render_pipeline& pl, const view_data& vp, const glm::mat4& rootTransform = glm::mat4( 1.0f ) ) const;

        void update( entity_list_t mEntities, const glm::mat4& rootTransform = glm::mat4( 1.0f ) );

        obb bounds( const glm::mat4& root ) const;

        bool destroy( void ) const;

        bool leaf( void ) const;
    };

    node::ptr_t mRoot;

    quad_hierarchy( obb bounds, const uint32_t maxDepth, entity_list_t entities );

    void update( entity_list_t entities );
};

INLINE obb quad_hierarchy::node::bounds( const glm::mat4& root ) const
{
    return std::move( obb( root * mLocalBounds.axes() ) );
}

INLINE bool quad_hierarchy::node::destroy( void ) const
{
    return mShouldDestroy;
}
