#include "map.h"
#include "view.h"
#include "input.h"

#include <iostream>
#include <random>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/projection.hpp>
#include <glm/gtx/string_cast.hpp>


// Return on false terminates merging for either the entire merger or just the adjacent being evaluated, respectively
using depth_merge_predicate_t = std::function< bool( shared_tile_region_t& m ) >;
using adjacent_merge_predicate_t = std::function< bool( shared_tile_region_t& m, shared_tile_region_t& adj ) >;

struct region_merge_predicates_t
{
    depth_merge_predicate_t entry; // called on entry of recursive call
    adjacent_merge_predicate_t adjacent; // called right before merge of adjacent region is about to occur.
};

namespace {
    std::random_device gRandDevice;
    std::mt19937 randEngine( gRandDevice() );
	std::uniform_int_distribution< uint32_t > wallDet( 0, 100 );
    std::uniform_int_distribution< uint16_t > randByte( 0, 255 );

    INLINE glm::vec4 make_random_color( void )
	{
		std::uniform_real_distribution< float > color( 0.5f, 1.0f );
		return glm::vec4( color( randEngine ),
						  color( randEngine ),
						  color( randEngine ),
						  1.0f );
	}

	const int32_t GEN_PASS_COUNT = 5;

	std::array< std::function< bool( uint32_t ) >, 4 > predicates =
	{{
		[]( uint32_t c )
		{
			return c <= 1;
		},
		[]( uint32_t c )
		{
			return c <= 2;
		},
		[]( uint32_t c )
		{
			UNUSEDPARAM( c );
			return false;
		},
		[]( uint32_t c )
		{
			UNUSEDPARAM( c );
			return false;
		}
	}};

    INLINE int32_t GridRange( int32_t x )
    {
        return glm::clamp( x, map_tile_generator::GRID_START, map_tile_generator::GRID_END - 1 );
    }

	INLINE int32_t tile_index( int32_t x, int32_t z )
    {
        return z * map_tile_generator::GRID_SIZE + x;
    }

	INLINE int32_t tile_clamp_index( int32_t x, int32_t z )
    {
        z = GridRange( z );
        x = GridRange( x );

		return tile_index( x, z );
    }

    bool MergeRegions( shared_tile_region_t& merged, map_tile_region* r0 )
    {
        if ( !merged ) return false;
        if ( !r0 ) return false;
        if ( r0->should_destroy() ) return false;

        for ( const map_tile* t: r0->mTiles )
        {
            t->owner( merged );
        }

        vector_insert_unique< const map_tile* >( merged->mTiles, r0->mTiles );
        vector_insert_unique< adjacent_region >( merged->mAdjacent, r0->mAdjacent );

        r0->mTiles.clear();
        r0->destroy();

        return true;
    }

    void Merge( shared_tile_region_t& merged,
                ref_tile_region_t region,
                region_merge_predicates_t predicates,
                const uint32_t currDepth,
                const uint32_t maxDepth )
    {
        assert( currDepth <= maxDepth );

        if ( !merged ) return;
        if ( !predicates.entry( merged ) ) return;
        if ( currDepth == maxDepth ) return;

        auto r = region.lock();
        if ( !r )
        {
            return;
        }

        if ( merged->mTiles.size() < r->mTiles.size() )
        {
            merged->mOrigin = r->mOrigin;
        }

        MergeRegions( merged, r.get() );

        std::vector< ref_tile_region_t > checkList;

        for ( auto i = r->mAdjacent.begin(); i != r->mAdjacent.end(); ++i )
        {
            {
                auto adj = ( *i ).mRegion.lock();

                if ( !adj )
                {
                    continue;
                }

                if ( !predicates.adjacent( merged, adj ) )
                {
                    continue;
                }
            }

            checkList.push_back( ( *i ).mRegion );

        }

        r->mAdjacent.clear();

        for ( ref_tile_region_t a: checkList )
        {
            Merge( merged, a, predicates, currDepth + 1, maxDepth );
        }
    }

    using iteration_fn_t = std::function< void( const map_tile& t, int32_t x, int32_t z ) >;

    void IterateTileRange( const map_tile& t, int32_t startOffset, int32_t endOffset, iteration_fn_t process )
    {
        int32_t add = 1;

        if ( startOffset > endOffset )
        {
            add = -1;
        }

        int32_t x0 = GridRange( t.mX + startOffset );
        int32_t z0 = GridRange( t.mZ + startOffset );

        int32_t xe = GridRange( t.mX + endOffset );
        int32_t ze = GridRange( t.mZ + endOffset );

        for ( int32_t z = z0; z <= ze; z += add )
        {
            for ( int32_t x = x0; x <= xe; x += add )
            {
                process( t, x, z );
            }
        }
    }

    void IterateTileListRange( std::vector< const map_tile* >& tiles, int32_t startOffset, int32_t endOffset, iteration_fn_t process )
    {
        for ( const map_tile* t: tiles )
        {
            IterateTileRange( *t, startOffset, endOffset, process );
        }
    }

    using integration_fn_t = std::function< void( float x, float fx, float gx ) >;

    void IntegrateTiles( const std::vector< const map_tile* >& functionTiles, integration_fn_t F )
    {
        uint32_t i = 0;

        while ( i < functionTiles.size() )
        {
            float fx = functionTiles[ i ]->mZ;
            float x = functionTiles[ i ]->mX;

            while ( i < functionTiles.size() && functionTiles[ i ]->mX == x )
                ++i;

            float gx = functionTiles[ i - 1 ]->mZ;

            F( x, fx, gx );
        }
    }

    // Origin of a region is basically it's centroid. We find it
    // using poor man's integration...
    bool ComputeOrigin( ref_tile_region_t region, const std::vector< map_tile >& tiles )
    {
        auto p = region.lock();
        if ( !p )
        {
            return false;
        }

        assert( !p->mTiles.empty() );

        // Sort the tiles in ascending order by their x values.
        // Duplicate x values are grouped together in a manner
        // where high z's come before lower z values.
        // So, for all x = 0 has a set of z values
        // where the first z values is the highest in that group
        // for x = 0.

        // The lowest and the highest z values represent f(x) and g(x), respectively.
        // We use our current set of values to act as distribution mechanism
        // with which we can integrate to find the area, and then
        // compute the centroid properly from.
        std::vector< const map_tile* > sorted( p->mTiles );
        std::sort( sorted.begin(), sorted.end(), []( const map_tile* a, const map_tile* b ) -> bool
        {
            if ( a->mX == b->mX )
            {
                return a->mZ > b->mZ;
            }

            return a->mX < b->mX;
        });

        float area = 0.0f;
        IntegrateTiles( sorted, [ &area ]( float x, float fx, float gx )
        {
            UNUSEDPARAM( x );
            area += fx - gx;
        });

        glm::vec2 v( 0.0f );
        IntegrateTiles( sorted, [ &v ]( float x, float fx, float gx )
        {
            v.x += x * ( fx - gx );
            v.y += ( fx + gx ) * 0.5f * ( fx - gx );
        });

        v /= area;

        // Flooring or ceiling v may or may not produce better values -- not sure.
        glm::ivec2 index( glm::round( v ) );

        assert( index.x < map_tile_generator::GRID_SIZE );
        assert( index.y < map_tile_generator::GRID_SIZE );

		p->mOrigin = &tiles[ tile_index( index.x, index.y ) ];

        return true;
    }

    obb ComputeBoundsFromRegion( const map_tile_region* r )
    {
        glm::ivec2 min( ( int32_t ) map_tile_generator::GRID_END ),
                   max( ( int32_t ) map_tile_generator::GRID_START );

        for ( const map_tile* t: r->mTiles )
        {
            if ( min.x > t->mX ) min.x = t->mX;
            if ( min.y > t->mZ ) min.y = t->mZ;
            if ( max.x < t->mX ) max.x = t->mX;
            if ( max.y < t->mZ ) max.y = t->mZ;
        }

        glm::vec2 s( max - min );

        float exp = log4( glm::max( s.x, s.y ) );

        float dim = glm::pow( 4, glm::ceil( exp ) );

        glm::vec3 size( dim, 1.0f, dim );

        glm::vec3 pos( r->mOrigin->mX * map_tile_generator::TRANSLATE_STRIDE,
                       0.0f,
                       r->mOrigin->mZ * map_tile_generator::TRANSLATE_STRIDE );

        pos.x -= map_tile_generator::TRANSLATE_STRIDE * 0.5f;
        pos.z -= map_tile_generator::TRANSLATE_STRIDE * 0.5f;

        glm::mat4 t( glm::translate( glm::mat4( 1.0f ), pos ) * glm::scale( glm::mat4( 1.0f ), size ) );

		return obb( t );
    }

    void PurgeFromAdjacent( std::vector< shared_tile_region_t >& regions, const shared_tile_region_t& target )
    {
        using predicate_t = std::function< bool( const adjacent_region& ) >;

        auto LEraseIf = [ &target ]( const adjacent_region& br ) -> bool
        {
            return br.mRegion.lock() == target;
        };

        for ( shared_tile_region_t& r: regions )
        {
            if ( r )
            {
                vector_erase_if< adjacent_region, predicate_t >( r->mAdjacent, LEraseIf );
            }
        }
    }

    // Basic assertions we run at the end of generation
    bool GeneratorTest( map_tile_generator& gen )
    {
        for ( ref_tile_region_t ref: gen.mRegions )
        {
            auto r = ref.lock();
            assert( r );
            assert( !r->should_destroy() );

            for ( const map_tile* t: r->mTiles )
            {
                assert( t->mType != map_tile::WALL );
                assert( t->owner() == ref );

                for ( ref_tile_region_t ref0: gen.mRegions )
                {
                    auto r0 = ref0.lock();

                    if ( r0 == r )
                    {
                        continue;
                    }

                    for ( const map_tile* t0: r0->mTiles )
                    {
                        if ( t == t0 )
                        {
                            return false;
                        }
                    }
                }
            }
        }

        return true;
    }
}

//-------------------------------------------------------------------------------------------------------
// bounds_region_t
//-------------------------------------------------------------------------------------------------------

uint32_t adjacent_region::count = 0;

adjacent_region::adjacent_region( void )
    : mID( count++ )
{
}

adjacent_region::adjacent_region( const adjacent_region& c )
    : mID( c.mID ),
      mTiles( c.mTiles ),
      mRegion( c.mRegion )
{
}

bool operator == ( const adjacent_region& a, const adjacent_region& b )
{
    return a.mRegion == b.mRegion;
}

bool operator != ( const adjacent_region& a, const adjacent_region& b )
{
    return !( a == b );
}

//-------------------------------------------------------------------------------------------------------
// tile_t
//-------------------------------------------------------------------------------------------------------

map_tile::map_tile( void )
    : entity( entity::BODY_DEPENDENT ),
      mType( map_tile::EMPTY ),
      mX( 0 ), mZ( 0 ),
      mHalfSpaceIndex( -1 )
{
}

void map_tile::set( const glm::mat4& transform )
{
	mBody.reset( new rigid_body( rigid_body::RESET_FORCE_ACCUM ) );

    switch ( mType )
    {
        case map_tile::BILLBOARD:
            mDepType = entity::BODY_DEPENDENT;
            mBody->set( transform );
            mBody->mass( 100.0f );


			add_bounds( ENTITY_BOUNDS_ALL, new obb( transform ) );

            break;

        default:
            if ( mType == map_tile::WALL )
            {
                add_bounds( ENTITY_BOUNDS_AIR_COLLIDE | ENTITY_BOUNDS_MOVE_COLLIDE,
                           new primitive_lookup( BOUNDS_PRIM_HALFSPACE, mHalfSpaceIndex ) );

				add_bounds( ENTITY_BOUNDS_AREA_EVAL, new obb( transform ) );

                mBody->mass( 10.0f );
            }
            else
            {
				add_bounds( ENTITY_BOUNDS_ALL, new obb( transform ) );
            }

            mDepType = entity::BOUNDS_DEPENDENT;
            break;
    }

    sync();
}

//-------------------------------------------------------------------------------------------------------
// tile_generator_t
//-------------------------------------------------------------------------------------------------------


map_tile_generator::map_tile_generator( collision_provider& collision_ )
    : mCollision( collision_ )
{
    mTiles.resize( GRID_SIZE * GRID_SIZE );

    for ( uint32_t pass = 0; pass < GEN_PASS_COUNT; ++pass )
	{
		for ( uint32_t z = 0; z < GRID_SIZE; ++z )
		{
			for ( uint32_t x = 0; x < GRID_SIZE; ++x )
			{
				int32_t index = tile_index( x, z );

                mTiles[ index ].mX = x;
                mTiles[ index ].mZ = z;

				make_tile( mTiles[ index ], pass );
			}
		}
	}

    // Iterate through each wall; any adjacent
    // tiles which aren't also walls will allow
    // for this wall to have a half-space
    // for the corresponding face.
    for ( map_tile* wall: mWalls )
    {
        collision_face_table_t halfSpaces;
        halfSpaces.fill( -1 );

        int32_t left = tile_index( wall->mX - 1, wall->mZ );
        int32_t forward = tile_index( wall->mX, wall->mZ - 1 );
        int32_t right = tile_index( wall->mX + 1, wall->mZ );
        int32_t back = tile_index( wall->mX, wall->mZ + 1 );

        bool hasHalfSpace = false;

        const obb& box = *ENTITY_PTR_GET_BOX( wall, ENTITY_BOUNDS_AREA_EVAL );

        if ( glm::ext::bound_range_max( left, 0, TABLE_SIZE ) && mTiles[ left ].mType != map_tile::WALL
             && ( wall->mX - 1 ) >= 0 )
        {
            halfSpaces[ COLLISION_FACE_LEFT ] = ( int32_t )mCollision.gen_half_space( box, COLLISION_FACE_LEFT );
            hasHalfSpace = true;
        }

        if ( glm::ext::bound_range_max( forward, 0, TABLE_SIZE ) && mTiles[ forward ].mType != map_tile::WALL
             && ( wall->mZ - 1 ) >= 0 )
        {
            halfSpaces[ COLLISION_FACE_FORWARD ] = ( int32_t )mCollision.gen_half_space( box, COLLISION_FACE_FORWARD );
            hasHalfSpace = true;
        }

        if ( glm::ext::bound_range_max( right, 0, TABLE_SIZE ) && mTiles[ right ].mType != map_tile::WALL
             && ( wall->mX + 1 ) < GRID_SIZE )
        {
            halfSpaces[ COLLISION_FACE_RIGHT ] = ( int32_t )mCollision.gen_half_space( box, COLLISION_FACE_RIGHT );
            hasHalfSpace = true;
        }

        if ( glm::ext::bound_range_max( back, 0, TABLE_SIZE ) && mTiles[ back ].mType != map_tile::WALL
             && ( wall->mZ + 1 ) < GRID_SIZE )
        {
            halfSpaces[ COLLISION_FACE_BACK ] = ( int32_t )mCollision.gen_half_space( box, COLLISION_FACE_BACK );
            hasHalfSpace = true;
        }

        if ( hasHalfSpace )
        {
			wall->mHalfSpaceIndex = ( int32_t ) mCollision.mHalfSpaceTable.size();

            wall->query_bounds( ENTITY_BOUNDS_MOVE_COLLIDE )
                    ->to_lookup()
					->mIndex = wall->mHalfSpaceIndex;

			mCollision.mHalfSpaceTable.push_back( halfSpaces );
        }
    }

    // Accumulate a list of tiles with halfSpaces, so we know
    // how to generate our regions for the map
    std::vector< const map_tile* > halfSpaceTiles;
    for ( const map_tile& tile: mTiles )
    {
        if ( tile.mHalfSpaceIndex >= 0 )
        {
            halfSpaceTiles.push_back( &tile );
        }
    }

    // Generate regions

    // Fill basic regions
    for ( const map_tile* hst: halfSpaceTiles )
    {
        bool res = find_regions( hst );
        assert( res );
    }

    // First pass
    find_adjacent_regions();

    {
        region_merge_predicates_t predicates;
        const uint32_t TILE_LIMIT = uint32_t( float( TABLE_SIZE ) * 0.15f );

        predicates.entry = [ TILE_LIMIT ]( shared_tile_region_t& m ) -> bool
        { return m->mTiles.size() <= TILE_LIMIT; };

        // No actual predicate necessary for our first merge pass
        predicates.adjacent = []( shared_tile_region_t& m, shared_tile_region_t& adj ) -> bool
        {
            UNUSEDPARAM( m );
            UNUSEDPARAM( adj );
            return true;
        };

        // Regions by default are very small, so we need to merge them together
        merge_regions( predicates, 6 );
    }

    // Merging kills all original regions (mostly), so we need to do this a second time
    find_adjacent_regions();

    {
        region_merge_predicates_t predicates;
        predicates.entry = []( shared_tile_region_t& m ) -> bool
        {
            UNUSEDPARAM( m );
            return true;
        };

        const uint32_t ADJ_TILE_LIMIT = uint32_t( float( TABLE_SIZE ) * 0.2f );
        const uint32_t M_TILE_LIMIT = uint32_t( float( TABLE_SIZE ) * 0.3f );

        predicates.adjacent = [ ADJ_TILE_LIMIT, M_TILE_LIMIT ]( shared_tile_region_t& m, shared_tile_region_t& adj ) -> bool
        {
            return ( adj->mTiles.size() < ADJ_TILE_LIMIT && m->mTiles.size() < M_TILE_LIMIT );
        };

        merge_regions( predicates, 4 );
    }

    find_adjacent_regions();

    purge_defunct_regions();

    using transfer_predicate_t = std::function< bool( map_tile* ) >;
    transfer_predicate_t LDoSwapIf = []( map_tile* t ) -> bool
    {
        bool needsSwap = !t->owned();

        if ( needsSwap )
        {
            t->mType = map_tile::WALL;
        }

        return needsSwap;
    };

    vector_transfer_if< map_tile*, transfer_predicate_t >( mWalls, mFreeSpace, LDoSwapIf );
    vector_transfer_if< map_tile*, transfer_predicate_t >( mWalls, mBillboards, LDoSwapIf );

    // Compute tile boundries for every region
    for ( ref_tile_region_t region: mRegions )
    {
        auto r = region.lock();
        assert( r );

        // NOTE: it's important to remember that, since we're searching [ x - 1, x + 1 ] and [ z - 1, z + 1 ]
        // entirely, that tiles which are only diagnolly adjacent to either a wall or a different region
        // will also be added. This may or may not be desired.
        IterateTileListRange( r->mTiles, -1, 1, [ this, &r ]( const map_tile& tile, int32_t x, int32_t z )
        {
			int32_t index = tile_clamp_index( x, z );
            auto r0 = mTiles[ index ].owner().lock();
            const map_tile* t = &mTiles[ index ];

            if ( r0 != r )
            {
                // if !r0, then we're next to a wall;
                // the regionTable isn't aware of wall tiles,
                // and thus has a nullptr for any index mapped to an area where a wall would be
                if ( r0 )
                {
                    assert( t->mType != map_tile::WALL );

                    adjacent_region* boundsRegion = r->find_adjacent_owner( t );
                    assert( boundsRegion );

                    boundsRegion->mTiles.push_back( &tile );
                }
                else
                {
                    assert( t->mType == map_tile::WALL );

                    // We'll be testing wall collisions
                    // in the region's quad tree, so it's important that
                    // we don't add any walls which lack half spaces
                    if ( t->mHalfSpaceIndex < 0 )
                    {
                        return;
                    }

                    // If we already have an adjacent_wall_t member, add it to the adjacency list.
                    // Otherwise, create a new one.
                    for ( adjacent_wall& w: r->mWalls )
                    {
                        if ( w.mSource == &tile )
                        {
                            w.mWalls.push_back( t );
                            return;
                        }
                    }

                    adjacent_wall w;
                    w.mSource = &tile;
                    w.mWalls.push_back( t );
                    w.mGoverningRegion = r;

                    r->mWalls.push_back( std::move( w ) );
                }
            }
        });
    }

    // Find an origin...
    // Do an integration...
    for ( ref_tile_region_t region: mRegions )
    {
        bool success = ComputeOrigin( region, mTiles );
        assert( success );

        if ( success )
        {
            auto r = region.lock();

            obb b = ComputeBoundsFromRegion( r.get() );

            uint32_t depth = ( uint32_t )glm::floor( log4( b.axes()[ 0 ][ 0 ] ) );

            r->mBoundsVolume.reset( new quad_hierarchy( std::move( b ), depth, r->entity_list() ) );
        }
    }

    assert( GeneratorTest( *this ) );
}

namespace {
    struct region_swap_entry_t
    {
        const map_tile* t;
        int32_t index;
    };
}

shared_tile_region_t map_tile_generator::fetch_region( const glm::vec3& p )
{
    int32_t x, z;
    get_tile_coords( x, z, p );

	auto pRegion = mTiles[ tile_index( x, z ) ].owner().lock();

    assert( pRegion );

    return pRegion;
}

bool map_tile_generator::find_regions( const map_tile* tile )
{
    if ( !tile )
    {
        return false;
    }

    if ( tile->mHalfSpaceIndex < 0 )
    {
        return false;
    }

    // Use the half-space index for the wall tile
    // by iterating through each half-space normal and finding tiles which lie in its
    // pathway until it hits a wall. Once it hits a wall,
    // we peace out and move onto the next normal. The result is
    // some regions which need to be "remixed" down the road.

	const collision_face_table_t& hst = mCollision.mHalfSpaceTable[ tile->mHalfSpaceIndex ];

    shared_tile_region_t regionPtr( new map_tile_region( tile ) );

    auto LSetTableEntry = []( shared_tile_region_t& region, const map_tile& t )
    {
        t.owner( region );
        region->mTiles.push_back( &t );
    };

    for ( int32_t i = 0; i < ( int32_t ) hst.size(); ++i )
    {
        if ( hst[ i ] < 0 )
        {
            continue;
        }

        int32_t j = ( i + 1 ) % hst.size();

        if ( hst[ j ] < 0 )
        {
            continue;
        }

        // Find the direction we need to move in...
		const glm::vec3& e1 = mCollision.mHalfSpaces[ hst[ i ] ].axes()[ 2 ];
		const glm::vec3& e2 = mCollision.mHalfSpaces[ hst[ j ] ].axes()[ 2 ];

        int32_t zp, xp;

        if ( e1.x != 0.0f )
        {
            xp = int32_t( e1.x );
        }
        else
        {
            zp = int32_t( e1.z );
        }

        if ( e2.x != 0.0f )
        {
            xp = int32_t( e2.x );
        }
        else
        {
            zp = int32_t( e2.z );
        }

        const map_tile* zWall = nullptr;
        const map_tile* xWall = nullptr;

        // if zp or xp is -1 then we need to move towards the beginning
        // of the grid for that axis, as opposed to towards the end.
        int32_t endZ = ( zp > GRID_START )? GRID_END: GRID_START - 1;
        int32_t endX = ( xp > GRID_START )? GRID_END: GRID_START - 1;

        // Move in that direction; a wall which is found in the X or Z direction marks the end,
        // of the X or Z value, respectively. For example, if x starts at 0, but has a wall at x = 5, z
        // starts at 0 and z has a wall at z = 5, then we have a region entirely within the area covered
        // by x = 5 and z = 5, from x = 0 and z = 0.
        for ( int32_t z = tile->mZ; z != endZ; z += zp )
        {
			const map_tile& ztest = mTiles[ tile_index( tile->mX, z ) ];

            if ( ztest.mType == map_tile::WALL && !zWall && &ztest != tile )
            {
                zWall = &ztest;
            }

            for ( int32_t x = tile->mX; x != endX; x += xp )
            {
				const map_tile& xtest = mTiles[ tile_index( x, tile->mZ ) ];

                if ( xtest.mType == map_tile::WALL && !xWall && &xtest != tile )
                {
                    xWall = &xtest;
                }

				int32_t index = tile_index( x, z );

                const map_tile& t = mTiles[ index ];

                if ( &t == xWall )
                {
                    break;
                }

                if ( &t == tile || t.mType == map_tile::WALL )
                {
                    continue;
                }

                // If a region is already active here,
                // it's important that we only swap tiles
                // if our regionPtr is closer to the tile than
                // the already active region is. If we swap
                // here only, though, there's also a chance
                // we may have to early out before we can
                // find other offenders, due to walls which
                // block the path of the halfspace's normal "ray".

                // So, we brute force this shit like a boss by
                // iterating over every tile held by pRegion->tiles
                // and performing the exact same distance test.
                // If we don't do this, it's possible to see
                // "pockets" of various regions lying within other
                // regions, which prevents them from being separate.

                std::vector< region_swap_entry_t > regionSwaps;
                map_tile_region* regionToSwap = nullptr;
                {
                    auto pRegion = mTiles[ index ].owner().lock();
                    if ( pRegion )
                    {
                        regionToSwap = pRegion.get();
                        for ( const map_tile* t0: pRegion->mTiles )
                        {
                            glm::vec2 o0( pRegion->mOrigin->mX, pRegion->mOrigin->mZ );
                            glm::vec2 o1( regionPtr->mOrigin->mX, regionPtr->mOrigin->mZ );
                            glm::vec2 p( ( float ) t0->mX, ( float ) t0->mZ );

                            if ( glm::distance( o1, p ) < glm::distance( o0, p ) )
                            {
								regionSwaps.push_back( { t0, tile_clamp_index( t0->mX, t0->mZ ) } );
                            }
                        }
                    }
                    else
                    {
                        LSetTableEntry( regionPtr, t );
                    }
                }

                for ( const region_swap_entry_t& e: regionSwaps )
                {
                    vector_remove_ptr< const map_tile* >( regionToSwap->mTiles, e.t );
                    LSetTableEntry( regionPtr, *e.t );
                }
            }

            // finding zWall implies a last iteration;
            // Since the x has finished, we're pretty much done here
            if ( zWall )
            {
                break;
            }
        }
    }

    mRegions.push_back( std::shared_ptr< map_tile_region >( regionPtr ) );

    return true;
}

void map_tile_generator::find_adjacent_regions( void )
{
    // Clear out any prior adjacent regions
    for ( ref_tile_region_t ref: mRegions )
    {
        auto p = ref.lock();
        if ( p )
        {
            p->mAdjacent.clear();
        }
    }

    // Find all regions which lie adjacent to each individual region
    for ( ref_tile_region_t region: mRegions )
    {
        adjacent_region_list_t unique;
        auto r0 = region.lock();
        assert( r0 );

        // For every tile in r0, look for adjacent tiles which reside in a different region than r0.
        IterateTileListRange( r0->mTiles, -1, 1, [ this, &r0, &unique ]( const map_tile& t, int32_t x, int32_t z )
        {
            UNUSEDPARAM( t );

			const ref_tile_region_t& rr = mTiles[ tile_index( x, z ) ].owner();

            bool addIt = false;
            {
                auto r1 = rr.lock();
                addIt = r1 && ( r1 != r0 ) && !r1->mTiles.empty();
            }

            if ( addIt )
            {
                adjacent_region br;
                br.mRegion = rr;

                unique.push_back( br );
            }
        });

        // Add the adjacent regions to r0's list
        for ( const adjacent_region& r: unique )
        {
            assert( r.mRegion != region );
            if ( !vector_contains( r0->mAdjacent, r ) )
            {
                r0->mAdjacent.push_back( r );
            }
        }
    }
}

void map_tile_generator::purge_defunct_regions( void )
{
    // Cleanup now defunct regions which have been merged.
    for ( auto r = mRegions.begin(); r != mRegions.end(); )
    {
        ref_tile_region_t wp = *r;

        auto p = wp.lock();

        if ( ( p && ( p->should_destroy() || p->mTiles.size() < MIN_REGION_SIZE ) ) || !p )
        {
            if ( p )
            {
                for ( const map_tile* t: p->mTiles )
                {
                    t->mOwner.reset();
                }
            }

            r = mRegions.erase( r );

            PurgeFromAdjacent( mRegions, p );
        }
        else
        {
            ++r;
        }
    }
}

void map_tile_generator::merge_regions( const region_merge_predicates_t& predicates, const uint32_t maxDepth )
{
    std::vector< shared_tile_region_t > merged;

    for ( ref_tile_region_t region: mRegions )
    {
        shared_tile_region_t merge( new map_tile_region() );

        Merge( merge, region, predicates, 0, maxDepth );

        if ( merge->mTiles.empty() )
        {
            continue;
        }

        merged.push_back( std::move( merge ) );
    }

    purge_defunct_regions();

    mRegions.insert( mRegions.end(), merged.begin(), merged.end() );
}

int32_t map_tile_generator::range_count( const map_tile& t, int32_t startOffset, int32_t endOffset )
{
    int32_t count = 0;

    IterateTileRange( t, startOffset, endOffset, [ this, &count ]( const map_tile& tile, int32_t x, int32_t z )
    {
        UNUSEDPARAM( tile );

		if ( mTiles[ tile_index( x, z ) ].mType != map_tile::EMPTY )
        {
            count++;
        }
    });

	return count;
}

void map_tile_generator::make_tile( map_tile& tile, int32_t pass )
{
    bool isWall;

	if ( pass == 0 )
	{
		isWall = wallDet( randEngine ) <= 40u;
	}
	else
	{
        isWall = range_count( tile, -1, 1 ) >= 5 || predicates[ pass - 1 ]( range_count( tile, -2, 2 ) );
	}

    tile.mBounds.reset();
    tile.mType = map_tile::EMPTY;

	// Multiply x and z values by 2 to accomodate for bounds scaling on the axis
	if ( isWall )
	{
        tile.mType = map_tile::WALL;
		if ( pass == GEN_PASS_COUNT - 1 )
		{
            mWalls.push_back( &tile );
		}
	}
	else
	{
		// There is a 5% chance that billboards will be added
		if ( wallDet( randEngine ) <= 5u )
		{
            tile.mType = map_tile::BILLBOARD;
			if ( pass == GEN_PASS_COUNT - 1 )
			{
                mBillboards.push_back( &tile );
			}

		}
		else
		{
			if ( pass == GEN_PASS_COUNT - 1 )
			{
                mFreeSpace.push_back( &tile );
			}
		}
	}

    if ( pass == GEN_PASS_COUNT - 1 )
	{
        tile.mSize = glm::vec3( 1.0f );

        tile.set( get_tile_transform( tile ) );
	}
}

namespace {

    INLINE void insert_appro_tile( map_tile& t,
                            map_tile_list_t& billboards,
                            map_tile_list_t& walls,
                            map_tile_list_t& freespace,
                            const view_frustum& frustum )
    {
        if ( !frustum.intersects( *ENTITY_GET_BOX( t, ENTITY_BOUNDS_AREA_EVAL ) ) )
        {
            return;
        }

        map_tile_list_t* pv = nullptr;

        switch ( t.mType )
        {
            case map_tile::WALL: pv = &walls; break;
            case map_tile::BILLBOARD: pv = &billboards; break;
            case map_tile::EMPTY: pv = &freespace; break;
        }

        if ( !vector_contains< map_tile* >( *pv, &t ) )
        {
            pv->push_back( &t );
        }
    }

    template < typename type_t >
    INLINE bool valid_position( type_t& x, type_t& y, const input_client& camera )
    {
        const view_data& viewParams = camera.view_params();

        get_tile_coords( x, y, viewParams.mOrigin );

        if ( x < type_t( map_tile_generator::GRID_START ) || y < type_t( map_tile_generator::GRID_START ) )
        {
            return false;
        }

        return true;
    }
}

void map_tile_generator::find_entities_raycast(
                                map_tile_list_t& billboards,
                                map_tile_list_t& walls,
                                map_tile_list_t& freespace,
                                const view_frustum& frustum,
                                const input_client& camera )
{
    float fCenterX, fCenterZ;
    if ( !valid_position( fCenterX, fCenterZ, camera ) )
    {
        return;
    }

    const view_data& viewParams = camera.view_params();

    glm::vec2 pos( fCenterX, fCenterZ );

    glm::vec3 u( camera.world_direction( G_DIR_FORWARD ) );
    glm::vec2 look( glm::normalize( glm::vec2( u.x, u.z ) ) );

    glm::vec3 v( glm::cross( u, G_DIR_UP ) );

    glm::vec2 plane( v.x, v.z );
    plane = glm::normalize( plane );

    int32_t half = int32_t( viewParams.mWidth * 0.25f );
    float invHalf = 1.0f / ( float )half;

    for ( int32_t x0 = -half; x0 <= half; ++x0 )
    {
        float camX = ( float ) x0 * invHalf;

        glm::vec2 dir( glm::normalize( look + plane * camX ) );

        glm::vec2 iT( glm::sqrt( 1 + ( dir.y * dir.y ) / ( dir.x * dir.x ) ),
                      glm::sqrt( 1 + ( dir.x * dir.x ) / ( dir.y * dir.y ) ) );

        glm::vec2 st;

        glm::vec2 step;

        glm::vec2 mapPos( glm::floor( pos ) );

        if ( dir.x < 0.0f )
        {
            st.x = ( pos.x - mapPos.x ) * iT.x;
            step.x = -1.0f;
        }
        else
        {
            st.x = ( mapPos.x + 1.0f - pos.x ) * iT.x;
            step.x = 1.0f;
        }

        if ( dir.y < 0.0f )
        {
            st.y = ( pos.y - mapPos.y ) * iT.y;
            step.y = -1.0f;
        }
        else
        {
            st.y = ( mapPos.y + 1.0f - pos.y ) * iT.y;
            step.y = 1.0f;
        }

        bool hit = false;

        int8_t t = 0;
        while ( !hit && t < 100 )
        {
            if ( st.x < st.y )
            {
                st.x += iT.x;
                mapPos.x += step.x;
            }
            else
            {
                st.y += iT.y;
                mapPos.y += step.y;
            }

			int32_t index = tile_clamp_index( mapPos.x, mapPos.y );

            if ( mTiles[ index ].mType == map_tile::WALL )
            {
                hit = true;
            }

            insert_appro_tile( mTiles[ index ],
                               billboards,
                               walls,
                               freespace,
                               frustum );

            t++;
        }
    }
}

void map_tile_generator::find_entities_radius( map_tile_list_t &billboards,
                                               map_tile_list_t &walls,
                                               map_tile_list_t &freespace,
                                               const view_frustum &frustum,
                                               const input_client &camera )
{
    int32_t centerX, centerZ;
    if ( !valid_position( centerX, centerZ, camera ) )
    {
        return;
    }

    const int32_t RADIUS = 10;
    const int32_t startX = centerX - RADIUS;
    const int32_t startZ = centerZ - RADIUS;
    const int32_t endX = centerX + RADIUS;
    const int32_t endZ = centerZ + RADIUS;

    for ( int32_t z = startZ; z < endZ; ++z )
    {
        if ( z >= ( int32_t ) GRID_SIZE )
        {
            break;
        }

        for ( int32_t x = startX; x < endX; ++x )
        {
            if ( x >= ( int32_t ) GRID_SIZE )
            {
                break;
            }

			int32_t index = tile_clamp_index( x, z );

            const obb& areaBox = *ENTITY_GET_BOX( mTiles[ index ], ENTITY_BOUNDS_AREA_EVAL );

            // cull frustum, insert into appropriate type, etc.
            if ( !frustum.intersects( areaBox ) )
            {
                continue;
            }

            switch ( mTiles[ index ].mType )
            {
                case map_tile::BILLBOARD:
                    billboards.push_back( &mTiles[ index ] );
                    break;
                case map_tile::WALL:
                    walls.push_back( &mTiles[ index ] );
                    break;
                case map_tile::EMPTY:
                    freespace.push_back( &mTiles[ index ] );
                    break;
            }
        }
    }
}

void map_tile_generator::find_entities(
                               map_tile_list_t& billboards,
                               map_tile_list_t& walls,
                               map_tile_list_t& freespace,
                               const view_frustum& frustum,
                               const input_client& camera )
{
    billboards.clear();
    walls.clear();
    freespace.clear();

#ifdef MAP_USE_RAYCAST
    find_entities_raycast( billboards,
                           walls,
                           freespace,
                           frustum,
                           camera );
#else
    find_entities_radius( billboards,
                          walls,
                          freespace,
                          frustum,
                          camera );
#endif
}

//-------------------------------------------------------------------------------------------------------
// tile_region_t
//-------------------------------------------------------------------------------------------------------

map_tile_region::map_tile_region( const map_tile* origin_ )
    : mDestroy( false ),
      mOrigin( origin_ ),
      mColor( make_random_color() ),
      mBoundsVolume( nullptr )
{
}

void map_tile_region::draw( const render_pipeline& pl, const view_data& vp )
{
    UNUSEDPARAM( pl );
    UNUSEDPARAM( vp );
}

void map_tile_region::destroy( void ) const
{
    mDestroy = true;
}

bool map_tile_region::should_destroy( void ) const
{
    return mDestroy;
}

adjacent_region* map_tile_region::find_adjacent_owner( const map_tile* t )
{
    for ( auto i = mAdjacent.begin(); i != mAdjacent.end(); ++i )
    {
        if ( vector_contains< const map_tile* >( ( *i ).mRegion.lock()->mTiles, t ) )
        {
            return &( *i );
        }
    }

    return nullptr;
}

quad_hierarchy::entity_list_t map_tile_region::entity_list( void ) const
{
    quad_hierarchy::entity_list_t entities;
    entities.resize( mTiles.size() );

    uint32_t i;
    for ( i = 0; i < entities.size(); ++i )
    {
        entities[ i ] = ( const entity* ) mTiles[ i ];
    }

    for ( const adjacent_wall& w: mWalls )
    {
        entities.resize( entities.size() + w.mWalls.size() );

        for ( const map_tile* wall: w.mWalls )
        {
            entities[ i++ ] = wall;
        }
    }

    return std::move( entities );
}


void map_tile_region::update( void )
{
    if ( !mBoundsVolume )
    {
        return;
    }

    mBoundsVolume->update( std::move( entity_list() ) );
}
