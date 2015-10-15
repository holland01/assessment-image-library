#include "entity.h"
#include "geom/geom.h"
#include "physics_world.h"


entity_bounds_primitive::entity_bounds_primitive( uint32_t flags, bounds_primitive* bounds_ )
    : usageFlags( flags ),
      bounds( bounds_ ),
      next( nullptr )
{
}

//-------------------------------------------------------------------------------------------------------
// entity_bounds_primitive_t
//-------------------------------------------------------------------------------------------------------

entity::entity(const glm::vec4& color )
    : mBounds( nullptr ),
      mColor( color ),
      mSize( 1.0f )
{
}

void entity::add_bounds( uint32_t usageFlags, bounds_primitive* newBounds )
{
    std::unique_ptr< entity_bounds_primitive >* b = &mBounds;

	while ( *b && ( *b )->usageFlags != usageFlags )
    {
		// Remove any potential duplicates for the use case we're now inserting
		( *b )->usageFlags &= ~usageFlags;
        b = &( *b )->next;
    }

    ( *b ).reset( new entity_bounds_primitive( usageFlags, newBounds ) );
}

namespace {

   INLINE bounds_primitive* __QueryBounds( entity_bounds_primitive* b, uint32_t flags )
    {
        while ( b )
        {
            if ( b->usageFlags & flags )
            {
                return b->bounds.get();
            }

            b = b->next.get();
        }

        return nullptr;
    }
}

bounds_primitive* entity::query_bounds( uint32_t flags )
{
    return __QueryBounds( mBounds.get(), flags );
}

const bounds_primitive* entity::query_bounds( uint32_t flags ) const
{
    return __QueryBounds( mBounds.get(), flags );
}

void entity::orient_to( const glm::vec3& v )
{
    /*
    obb* bounds = query_bounds( ENTITY_BOUNDS_AREA_EVAL )->to_box();

    if ( !bounds )
    {
        return;
    }

	glm::vec3 boundsOrigin( bounds->origin() );

    glm::vec3 dir( v - boundsOrigin );
    dir.y = 0.0f;
    dir = glm::normalize( dir );

    glm::mat3 orient(
        orient_by_direction(
                dir,
                glm::vec3( 0.0f, 0.0f, 1.0f ),
                glm::vec3( -1.0f, 0.0f, 0.0f )
        )
    );

    bounds->axes( orient );
    */

    UNUSEDPARAM( v );
}

void entity::sync( void )
{
    if ( !mPhysEnt || !mPhysEnt->mMotionState )
        return;
}
