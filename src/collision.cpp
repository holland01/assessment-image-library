#include "collision.h"
#include "geom.h"
#include <glm/glm.hpp>

//-------------------------------------------------------------------------------------------------------
// collision_entity_t
//-------------------------------------------------------------------------------------------------------

collision_entity_t::collision_entity_t( std::weak_ptr< entity_t > a_, std::weak_ptr< entity_t > b_ )
    : a( a_ ),
      b( b_ ),
      colliding( false ),
      normal( 0.0f )
{
}

//-------------------------------------------------------------------------------------------------------
// collision_provider_t
//-------------------------------------------------------------------------------------------------------

uint32_t collision_provider_t::GenHalfSpace( const bounding_box_t& bounds, collision_face_t face )
{
    std::array< glm::vec3, NUM_COLLISION_FACES > halfSpaceNormals =
    {{
        glm::vec3( -1.0f, 0.0f, 0.0f ),
        glm::vec3( 0.0f, 0.0f, -1.0f ),
        glm::vec3( 1.0f, 0.0f, 0.0f ),
        glm::vec3( 0.0f, 0.0f, 1.0f )
    }};

    uint32_t index = ( int32_t ) halfSpaces.size();

    half_space_t hs( bounds, halfSpaceNormals[ face ] );
    halfSpaces.push_back( std::move( hs ) );

    return index;
}

bool collision_provider_t::BoundsCollidesHalfSpace( collision_entity_t& ce ) const
{
    UNUSEDPARAM( ce );
    return true;
}
