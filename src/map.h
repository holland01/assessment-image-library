#pragma once

#include "def.h"
#include "entity.h"
#include "physics.h"
#include "geom.h"
#include "renderer.h"

#include <set>
#include <unordered_set>

struct frustum_t;

struct tile_t;
struct tile_region_t;

//struct bounds_region_t;

using billboard_list_t = std::vector< tile_t* >;
using freespace_list_t = std::vector< const tile_t* >;
using wall_list_t = std::vector< const tile_t* >;

using ref_tile_region_t = std::weak_ptr< tile_region_t >;
using shared_tile_region_t = std::shared_ptr< tile_region_t >;

struct region_merge_predicates_t;

//-------------------------------------------------------------------------------------------------------
// bounds_region_t
//-------------------------------------------------------------------------------------------------------

struct bounds_region_t
{
public:
    using hash_type_t = size_t;

private:
    static const hash_type_t UNKNOWN = UINTMAX_MAX;
    static hash_type_t count;
    hash_type_t id;

public:
    std::vector< const tile_t* > tiles; // tiles touching the given region from within another region
    ref_tile_region_t region; // Region which the tiles are adjacent to

    bounds_region_t( void );

    bounds_region_t( const bounds_region_t& c );
    bounds_region_t( bounds_region_t&& m ) = delete;

    hash_type_t GetID( void ) const;
};

INLINE bounds_region_t::hash_type_t bounds_region_t::GetID( void ) const
{
    return id;
}

bool operator == ( const bounds_region_t& a, const bounds_region_t& b );

bool operator != ( const bounds_region_t& a, const bounds_region_t& b );

using bounds_region_set_t = std::vector< bounds_region_t >;

namespace std {
    template <> struct hash< bounds_region_t >
    {
        size_t operator()( const bounds_region_t& x ) const
        {
            return hash< bounds_region_t::hash_type_t >()( x.GetID() );
        }
    };
}

//-------------------------------------------------------------------------------------------------------
// tile_t
//-------------------------------------------------------------------------------------------------------

struct tile_t: public entity_t
{    
    using ptr_t = std::shared_ptr< tile_t >;

    friend struct tile_generator_t;

private:

    mutable ref_tile_region_t owner;

public:

    enum type_t
    {
        BILLBOARD,
        WALL,
        EMPTY
    };

    type_t type;
	int32_t x, z, halfSpaceIndex;
    float size; // does not bear any relation to index lookup in the table at all.

	tile_t( void );

    void Set( const glm::mat4& transform );

    void SetOwner( shared_tile_region_t& r ) const;

    const ref_tile_region_t& GetOwner( void ) const;

    bool HasOwner( void ) const;
};

INLINE void tile_t::SetOwner( shared_tile_region_t& r ) const
{
    owner = r;
}

INLINE const ref_tile_region_t& tile_t::GetOwner( void ) const
{
    return owner;
}

INLINE bool tile_t::HasOwner( void ) const
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

	std::vector< tile_t > tiles;
    std::vector< shared_tile_region_t > regions;

    billboard_list_t billboards;
    wall_list_t walls;
    freespace_list_t freeSpace;

    using merge_predicate_fn_t = std::function< bool( shared_tile_region_t& m ) >;

	std::vector< half_space_table_t > halfSpaceTable;
	std::vector< half_space_t > halfSpaces;

    tile_generator_t( void );

    bool HasRegion( const tile_region_t* r ) const;

    bool FindRegions( const tile_t* tile );

    void FindAdjacentRegions( void );

    void PurgeDefunctRegions( void );

    void MergeRegions( const region_merge_predicates_t& predicates, const uint32_t maxDepth );

    void SetTile( int32_t pass,
                  int32_t x,
                  int32_t z,
				  std::vector< tile_t* >& mutWalls );

    int32_t RangeCount( int32_t x, int32_t z, int32_t offsetEnd );

	bool CollidesWall( glm::vec3& normal, const tile_t& t, const bounding_box_t& bounds, half_space_t& outHalfSpace );

    void GetEntities( billboard_list_t& billboards,
                      wall_list_t& walls,
                      freespace_list_t& freeSpace,
					  const frustum_t& frustum,
                      const view_params_t& viewParams );

	half_space_t GenHalfSpace( const tile_t& t, const glm::vec3& normal );
};

//-------------------------------------------------------------------------------------------------------
// tile_region_t
//-------------------------------------------------------------------------------------------------------

struct tile_region_t
{
private:
    mutable bool destroy;

public:

    const tile_t* origin;

    glm::vec4 color;

    std::vector< const tile_t* > tiles;
    std::vector< const tile_t* > wallTiles; // tiles which touch walls
    std::vector< const tile_t* > boundsTiles; // tiles which touch adjacent regions
    bounds_region_set_t adjacent;

    tile_region_t( const tile_t* origin = nullptr );

    void Draw( const pipeline_t& pl, const view_params_t& vp );

    void Destroy( void ) const;

    bool ShouldDestroy( void ) const;
};

//-------------------------------------------------------------------------------------------------------
// quad_hierarchy_t
//
// | II.  | I. |
// | III. | IV. |
//-------------------------------------------------------------------------------------------------------

struct quad_hierarchy_t
{
    static const uint8_t NODE_COUNT = 4;

    using entity_list_t = std::vector< std::weak_ptr< entity_t > >;

    struct node_t
    {
        using ptr_t = std::unique_ptr< node_t >;

        bounding_box_t bounds;

        entity_list_t entities;

        std::array< ptr_t, NODE_COUNT > children;

        node_t( uint32_t curDepth, const uint32_t maxDepth, bounding_box_t bounds );

        void Draw( const pipeline_t& pl, const view_params_t& vp, const glm::mat4& rootTransform = glm::mat4( 1.0f ) ) const;
    };

    node_t::ptr_t root;
};

