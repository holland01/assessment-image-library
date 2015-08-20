#pragma once

#include "def.h"
#include "entity.h"
#include "physics.h"
#include "geom.h"
#include "renderer.h"
#include "bvh.h"

#include <set>
#include <unordered_set>

struct frustum_t;

struct map_tile_t;
struct tile_region_t;

using map_tile_list_t = std::vector< map_tile_t* >;

using ref_tile_region_t = std::weak_ptr< tile_region_t >;
using shared_tile_region_t = std::shared_ptr< tile_region_t >;

struct region_merge_predicates_t;

//-------------------------------------------------------------------------------------------------------
// bounds_region_t
//-------------------------------------------------------------------------------------------------------

struct bounds_region_t
{
    static const uint32_t UNKNOWN = UINT32_MAX;
    static uint32_t count;
    uint32_t id;

    std::vector< const map_tile_t* > tiles; // tiles touching the given region from within another region
    ref_tile_region_t region; // Region which the tiles are adjacent to

    bounds_region_t( void );
    bounds_region_t( const bounds_region_t& c );
};

bool operator == ( const bounds_region_t& a, const bounds_region_t& b );

bool operator != ( const bounds_region_t& a, const bounds_region_t& b );

using bounds_region_list_t = std::vector< bounds_region_t >;

//-------------------------------------------------------------------------------------------------------
// tile_t
//-------------------------------------------------------------------------------------------------------

struct map_tile_t: public entity_t
{    
    using ptr_t = std::shared_ptr< map_tile_t >;

    friend struct tile_generator_t;

private:

    mutable ref_tile_region_t owner;

public:

    enum type_t
    {
        BILLBOARD = 0,
        WALL,
        EMPTY
    };

    type_t type;
	int32_t x, z, halfSpaceIndex;
    float size; // does not bear any relation to index lookup in the table at all.

    map_tile_t( void );

    void Set( const glm::mat4& transform );

    void SetOwner( shared_tile_region_t& r ) const;

    const ref_tile_region_t& GetOwner( void ) const;

    bool HasOwner( void ) const;
};

INLINE void map_tile_t::SetOwner( shared_tile_region_t& r ) const
{
    owner = r;
}

INLINE const ref_tile_region_t& map_tile_t::GetOwner( void ) const
{
    return owner;
}

INLINE bool map_tile_t::HasOwner( void ) const
{
    return !owner.expired();
}

//-------------------------------------------------------------------------------------------------------
// tile_generator_t
//-------------------------------------------------------------------------------------------------------

struct tile_generator_t
{	
public:
    static const int32_t GRID_SIZE = 100;

    static const int32_t TABLE_SIZE = GRID_SIZE * GRID_SIZE;

    static const int32_t GRID_START = 0;

    static const int32_t GRID_END = GRID_SIZE;

    static constexpr int32_t MIN_REGION_SIZE = int32_t( float( TABLE_SIZE ) * 0.01f );

    static constexpr float TRANSLATE_STRIDE = 2.0f; // for rendering

    using region_table_t = std::array< ref_tile_region_t, TABLE_SIZE >;

	enum faceIndex_t
	{
		FACE_LEFT = 0,
		FACE_FORWARD,
		FACE_RIGHT,
		FACE_BACK,
		NUM_FACES
	};

	using half_space_table_t = std::array< int32_t, NUM_FACES >;

    std::vector< map_tile_t > tiles;

    std::vector< shared_tile_region_t > regions;

    map_tile_list_t billboards;

    map_tile_list_t walls;

    map_tile_list_t freeSpace;

    using merge_predicate_fn_t = std::function< bool( shared_tile_region_t& m ) >;

	std::vector< half_space_table_t > halfSpaceTable;

    std::vector< half_space_t > halfSpaces;

                tile_generator_t( void );

    bool        FindRegions( const map_tile_t* tile );

    void        FindAdjacentRegions( void );

    void        PurgeDefunctRegions( void );

    void        MergeRegions( const region_merge_predicates_t& predicates, const uint32_t maxDepth );

    void        SetTile( map_tile_t& tile, int32_t pass );

    int32_t     RangeCount( const map_tile_t& t, int32_t startOffset, int32_t offsetEnd );

    bool        CollidesWall( glm::vec3& normal, const map_tile_t& t, const bounding_box_t& bounds, half_space_t& outHalfSpace );

    void        GetEntities( map_tile_list_t& billboards,
                              map_tile_list_t& walls,
                              map_tile_list_t& freeSpace,
                              const frustum_t& frustum_t,
                              const view_params_t& viewParams );

    void         GenHalfSpacesFromWalls( map_tile_list_t& wallTiles );

    half_space_t GenHalfSpace( const map_tile_t& t, const glm::vec3& normal );
};

//-------------------------------------------------------------------------------------------------------
// tile_region_t
//-------------------------------------------------------------------------------------------------------

struct quad_hierarchy_t;

struct tile_region_t
{
private:
    mutable bool destroy;

public:

    const map_tile_t* origin;

    glm::vec4 color;

    std::vector< const map_tile_t* > tiles;

    std::vector< const map_tile_t* > wallTiles; // tiles which touch walls

    bounds_region_list_t adjacent;

    quad_hierarchy_t::ptr_t boundsVolume;

    tile_region_t( const map_tile_t* origin = nullptr );

    void Draw( const pipeline_t& pl, const view_params_t& vp );

    void Destroy( void ) const;

    bool ShouldDestroy( void ) const;

    bounds_region_t* FindAdjacentOwner( const map_tile_t* t );

    quad_hierarchy_t::entity_list_t GetEntityList( void ) const;

    void Update( void );
};



