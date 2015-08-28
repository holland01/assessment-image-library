#pragma once

#include "def.h"
#include "entity.h"
#include "collision.h"
#include "physics.h"
#include "geom.h"
#include "renderer.h"
#include "bvh.h"

#include <set>
#include <unordered_set>

struct view_frustum;
struct input_client;

struct map_tile;
struct map_tile_region;

using map_tile_list_t = std::vector< map_tile* >;

using ref_tile_region_t = std::weak_ptr< map_tile_region >;
using shared_tile_region_t = std::shared_ptr< map_tile_region >;

struct region_merge_predicates_t;

//-------------------------------------------------------------------------------------------------------
// adjacent_region_t
//-------------------------------------------------------------------------------------------------------

struct adjacent_region
{
    static const uint32_t UNKNOWN = UINT32_MAX;

    static uint32_t count;

    uint32_t mID;

    std::vector< const map_tile* > mTiles;

    ref_tile_region_t mRegion;

    adjacent_region( void );

    adjacent_region( const adjacent_region& c );
};

bool operator == ( const adjacent_region& a, const adjacent_region& b );

bool operator != ( const adjacent_region& a, const adjacent_region& b );

using adjacent_region_list_t = std::vector< adjacent_region >;

//-------------------------------------------------------------------------------------------------------
// adjacent_wall_t
//-------------------------------------------------------------------------------------------------------

struct adjacent_wall
{
    const map_tile* mSource;

    ref_tile_region_t mGoverningRegion;

    std::vector< const map_tile* > mWalls;

    adjacent_wall( void )
        : mSource( nullptr )
    {
    }
};

//-------------------------------------------------------------------------------------------------------
// tile_t
//-------------------------------------------------------------------------------------------------------

struct map_tile: public entity
{    
    using ptr_t = std::shared_ptr< map_tile >;

    friend struct map_tile_generator;

private:

    mutable ref_tile_region_t mOwner;

public:
    enum map_tile_type
    {
        BILLBOARD = 0,
        WALL,
        EMPTY
    };

    map_tile_type mType;
    int32_t mX, mZ, mHalfSpaceIndex;
    float mSize; // does not bear any relation to index lookup in the table at all.

    map_tile( void );

    void set( const glm::mat4& transform );

    void owner( shared_tile_region_t& r ) const;

    const ref_tile_region_t& owner( void ) const;

    bool owned( void ) const;
};

INLINE void map_tile::owner( shared_tile_region_t& r ) const
{
    mOwner = r;
}

INLINE const ref_tile_region_t& map_tile::owner( void ) const
{
    return mOwner;
}

INLINE bool map_tile::owned( void ) const
{
    return !mOwner.expired();
}

//-------------------------------------------------------------------------------------------------------
// tile_generator_t
//-------------------------------------------------------------------------------------------------------

struct map_tile_generator
{	
private:
    void find_entities_raycast( map_tile_list_t& billboards,
                                map_tile_list_t& walls,
                                map_tile_list_t& freespace,
                                const view_frustum& frustum,
                                const input_client& camera );

    void find_entities_radius( map_tile_list_t& billboards,
                                map_tile_list_t& walls,
                                map_tile_list_t& freespace,
                                const view_frustum& frustum,
                                const input_client& camera );

public:
    static const int32_t GRID_SIZE = 100;

    static const int32_t TABLE_SIZE = GRID_SIZE * GRID_SIZE;

    static const int32_t GRID_START = 0;

    static const int32_t GRID_END = GRID_SIZE;

    static constexpr int32_t MIN_REGION_SIZE = int32_t( float( TABLE_SIZE ) * 0.01f );

    static constexpr float TRANSLATE_STRIDE = 2.0f; // for rendering

    using region_table_t = std::array< ref_tile_region_t, TABLE_SIZE >;

    std::vector< map_tile > mTiles;

    std::vector< shared_tile_region_t > mRegions;

    map_tile_list_t mBillboards;

    map_tile_list_t mWalls;

    map_tile_list_t mFreeSpace;

    collision_provider& mCollision;

    using merge_predicate_fn_t = std::function< bool( shared_tile_region_t& m ) >;

                map_tile_generator( collision_provider& mCollision );

    shared_tile_region_t    fetch_region( const glm::vec3& p );

    bool                    find_regions( const map_tile* tile );

    void                    find_adjacent_regions( void );

    void                    purge_defunct_regions( void );

    void                    merge_regions( const region_merge_predicates_t& predicates, const uint32_t maxDepth );

    void                    tile( map_tile& tile, int32_t pass );

    int32_t                 range_count( const map_tile& t, int32_t startOffset, int32_t offsetEnd );

    bool                    collides_wall( glm::vec3& normal, const map_tile& t, const obb& bounds, halfspace& outHalfSpace );

    void                    find_entities(map_tile_list_t& mBillboards,
                                          map_tile_list_t& mWalls,
                                          map_tile_list_t& mFreeSpace,
                                          const view_frustum& view_frustum,
                                          const input_client& camera );

    const map_tile_list_t& walls( void ) const { return mWalls; }
};

//-------------------------------------------------------------------------------------------------------
// tile_region_t
//-------------------------------------------------------------------------------------------------------

struct quad_hierarchy;

struct map_tile_region
{
private:

    mutable bool mDestroy;

public:

    const map_tile* mOrigin;

    glm::vec4 mColor;

    std::vector< const map_tile* > mTiles;

    std::vector< adjacent_wall > mWalls; // tiles which touch walls

    adjacent_region_list_t mAdjacent;

    quad_hierarchy::ptr_t mBoundsVolume;

    map_tile_region( const map_tile* mOrigin = nullptr );

    void draw( const render_pipeline& pl, const view_data& vp );

    void destroy( void ) const;

    bool should_destroy( void ) const;

    adjacent_region* find_adjacent_owner( const map_tile* t );

    quad_hierarchy::entity_list_t entity_list( void ) const;

    void update( void );
};

//-------------------------------------------------------------------------------------------------------
// global
//-------------------------------------------------------------------------------------------------------

template< typename type_t >
static INLINE void get_tile_coords( type_t& x, type_t& z, const glm::vec3& v );

static INLINE glm::mat4 get_tile_transform( const map_tile& t );

#include "map.inl"


template< typename type_t >
static INLINE void get_tile_coords( type_t& x, type_t& z, const glm::vec3& v )
{
   x = ( type_t )( v.x * 0.5f );
   z = ( type_t )( v.z * 0.5f );
}

static INLINE glm::mat4 get_tile_transform( const map_tile& tile )
{
    glm::mat4 t( glm::translate( glm::mat4( 1.0f ),
                 glm::vec3( map_tile_generator::TRANSLATE_STRIDE * tile.mX,
                            0.0f,
                            map_tile_generator::TRANSLATE_STRIDE * tile.mZ ) ) );

    return std::move( t * tile.scale_transform() );
}


