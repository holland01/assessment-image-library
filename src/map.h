#pragma once

#include "def.h"
#include "entity.h"
#include "physics.h"
#include "geom.h"
#include "renderer.h"

struct frustum_t;

struct tile_t;
struct tile_region_t;

using billboard_list_t = std::vector< tile_t* >;
using freespace_list_t = std::vector< const tile_t* >;
using wall_list_t = std::vector< const tile_t* >;

//-------------------------------------------------------------------------------------------------------
// tile_t
//-------------------------------------------------------------------------------------------------------

struct tile_t : public entity_t
{    
    using ptr_t = std::shared_ptr< tile_t >;

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
};

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
    std::vector< std::shared_ptr< tile_region_t > > regions;

    billboard_list_t billboards;
    wall_list_t walls;
    freespace_list_t freeSpace;

	texture_t billTexture;

	std::vector< half_space_table_t > halfSpaceTable;
	std::vector< half_space_t > halfSpaces;

    tile_generator_t( void );

    bool HasRegion( const tile_region_t* r ) const;

    bool FindRegions( const tile_t* tile, std::array< tile_region_t*, TABLE_SIZE >& regionTable );

    void SetTile( int32_t pass,
                  int32_t x,
                  int32_t z,
				  std::vector< tile_t* >& mutWalls );

    int32_t RangeCount( int32_t x, int32_t z, int32_t offsetEnd );

    int32_t TileIndex( int32_t x, int32_t z ) const;
    int32_t TileModIndex( int32_t x, int32_t z ) const;

	bool CollidesWall( glm::vec3& normal, const tile_t& t, const bounding_box_t& bounds, half_space_t& outHalfSpace );

    void GetEntities( billboard_list_t& billboards,
                      wall_list_t& walls,
                      freespace_list_t& freeSpace,
					  const frustum_t& frustum,
                      const view_params_t& viewParams );

	half_space_t GenHalfSpace( const tile_t& t, const glm::vec3& normal );
};

INLINE int32_t tile_generator_t::TileIndex( int32_t x, int32_t z ) const
{
	return z * GRID_SIZE + x;
}

INLINE int32_t tile_generator_t::TileModIndex( int32_t x, int32_t z ) const
{
    return ( glm::abs( z ) % GRID_SIZE ) * GRID_SIZE + glm::abs( x ) % GRID_SIZE;
}

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
    std::vector< const tile_region_t* > adjacent;

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

