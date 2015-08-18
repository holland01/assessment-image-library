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

struct quad_hierarchy_t
{
    using ptr_t = std::unique_ptr< quad_hierarchy_t >;

    static const uint8_t NODE_COUNT = 4;

    struct node_t
    {
        using ptr_t = std::unique_ptr< node_t >;

        bounding_box_t bounds;

        std::vector< std::weak_ptr< entity_t > > entities;

        std::array< ptr_t, NODE_COUNT > children;

        node_t( uint32_t curDepth, const uint32_t maxDepth, bounding_box_t bounds );

        void Draw( const pipeline_t& pl, const view_params_t& vp, const glm::mat4& rootTransform = glm::mat4( 1.0f ) ) const;

        void Update( const std::vector< std::weak_ptr< entity_t > >& entities, const glm::mat4& rootTransform = glm::mat4( 1.0f ) );
    };

    node_t::ptr_t root;

    quad_hierarchy_t( bounding_box_t bounds, const uint32_t maxDepth );
};
