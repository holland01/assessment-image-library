#include "map.h"
#include "view.h"

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
	std::random_device randDevice;
	std::mt19937 randEngine( randDevice() );
	std::uniform_int_distribution< uint32_t > wallDet( 0, 100 );
    std::uniform_int_distribution< uint16_t > randByte( 0, 255 );

	INLINE glm::vec4 RandomColor( void )
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
        return glm::clamp( x, tile_generator_t::GRID_START, tile_generator_t::GRID_END - 1 );
    }

    INLINE int32_t TileIndex( int32_t x, int32_t z )
    {
        return z * tile_generator_t::GRID_SIZE + x;
    }

    INLINE int32_t TileModIndex( int32_t x, int32_t z )
    {
        z = GridRange( z );
        x = GridRange( x );

        return TileIndex( x, z );
    }

    bool MergeRegions( shared_tile_region_t& merged, tile_region_t* r0 )
    {
        if ( !merged ) return false;
        if ( !r0 ) return false;
        if ( r0->ShouldDestroy() ) return false;

        for ( const map_tile_t* t: r0->tiles )
        {
            t->SetOwner( merged );
        }

        Vector_InsertUnique< const map_tile_t* >( merged->tiles, r0->tiles );
        Vector_InsertUnique< bounds_region_t >( merged->adjacent, r0->adjacent );

        r0->tiles.clear();
        r0->Destroy();

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

        if ( merged->tiles.size() < r->tiles.size() )
        {
            merged->origin = r->origin;
        }

        MergeRegions( merged, r.get() );

        std::vector< ref_tile_region_t > checkList;

        for ( auto i = r->adjacent.begin(); i != r->adjacent.end(); ++i )
        {
            {
                auto adj = ( *i ).region.lock();

                if ( !adj )
                {
                    continue;
                }

                if ( !predicates.adjacent( merged, adj ) )
                {
                    continue;
                }
            }

            checkList.push_back( ( *i ).region );

        }

        r->adjacent.clear();

        for ( ref_tile_region_t a: checkList )
        {
            Merge( merged, a, predicates, currDepth + 1, maxDepth );
        }
    }

    using iteration_fn_t = std::function< void( const map_tile_t& t, int32_t x, int32_t z ) >;

    void IterateTileRange( const map_tile_t& t, int32_t startOffset, int32_t endOffset, iteration_fn_t process )
    {
        int32_t add = 1;

        if ( startOffset > endOffset )
        {
            add = -1;
        }

        int32_t x0 = GridRange( t.x + startOffset );
        int32_t z0 = GridRange( t.z + startOffset );

        int32_t xe = GridRange( t.x + endOffset );
        int32_t ze = GridRange( t.z + endOffset );

        for ( int32_t z = z0; z <= ze; z += add )
        {
            for ( int32_t x = x0; x <= xe; x += add )
            {
                process( t, x, z );
            }
        }
    }

    void IterateTileListRange( std::vector< const map_tile_t* >& tiles, int32_t startOffset, int32_t endOffset, iteration_fn_t process )
    {
        for ( const map_tile_t* t: tiles )
        {
            IterateTileRange( *t, startOffset, endOffset, process );
        }
    }

    using integration_fn_t = std::function< void( float x, float fx, float gx ) >;

    void IntegrateTiles( const std::vector< const map_tile_t* >& functionTiles, integration_fn_t F )
    {
        uint32_t i = 0;

        while ( i < functionTiles.size() )
        {
            float fx = functionTiles[ i ]->z;
            float x = functionTiles[ i ]->x;

            while ( i < functionTiles.size() && functionTiles[ i ]->x == x )
                ++i;

            float gx = functionTiles[ i - 1 ]->z;

            F( x, fx, gx );
        }
    }

    // Origin of a region is basically it's centroid. We find it
    // using poor man's integration...
    bool ComputeOrigin( ref_tile_region_t region, const std::vector< map_tile_t >& tiles )
    {
        auto p = region.lock();
        if ( !p )
        {
            return false;
        }

        assert( !p->tiles.empty() );

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
        std::vector< const map_tile_t* > sorted( p->tiles );
        std::sort( sorted.begin(), sorted.end(), []( const map_tile_t* a, const map_tile_t* b ) -> bool
        {
            if ( a->x == b->x )
            {
                return a->z > b->z;
            }

            return a->x < b->x;
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

        assert( index.x < tile_generator_t::GRID_SIZE );
        assert( index.y < tile_generator_t::GRID_SIZE );

        p->origin = &tiles[ TileIndex( index.x, index.y ) ];

        return true;
    }

    bounding_box_t ComputeBoundsFromRegion( const tile_region_t* r )
    {
        glm::ivec2 min( ( int32_t ) tile_generator_t::GRID_END ),
                   max( ( int32_t ) tile_generator_t::GRID_START );

        for ( const map_tile_t* t: r->tiles )
        {
            if ( min.x > t->x ) min.x = t->x;
            if ( min.y > t->z ) min.y = t->z;
            if ( max.x < t->x ) max.x = t->x;
            if ( max.y < t->z ) max.y = t->z;
        }

        glm::vec2 s( max - min );

        glm::vec3 size( s.x, 1.0f, s.y );

        glm::vec3 pos( r->origin->x * tile_generator_t::TRANSLATE_STRIDE,
                       0.0f,
                       r->origin->z * tile_generator_t::TRANSLATE_STRIDE );

        pos.x -= tile_generator_t::TRANSLATE_STRIDE * 0.5f;
        pos.z -= tile_generator_t::TRANSLATE_STRIDE * 0.5f;

        glm::mat4 t( glm::translate( glm::mat4( 1.0f ), pos ) * glm::scale( glm::mat4( 1.0f ), size ) );

        return bounding_box_t( t );
    }

    void PurgeFromAdjacent( std::vector< shared_tile_region_t >& regions, const shared_tile_region_t& target )
    {
        using predicate_t = std::function< bool( const bounds_region_t& ) >;

        auto LEraseIf = [ &target ]( const bounds_region_t& br ) -> bool
        {
            return br.region.lock() == target;
        };

        for ( shared_tile_region_t& r: regions )
        {
            if ( r )
            {
                Vector_EraseIf< bounds_region_t, predicate_t >( r->adjacent, LEraseIf );
            }
        }
    }

    // Basic assertions we run at the end of generation
    bool GeneratorTest( tile_generator_t& gen )
    {
        for ( ref_tile_region_t ref: gen.regions )
        {
            auto r = ref.lock();
            assert( r );
            assert( !r->ShouldDestroy() );

            for ( const map_tile_t* t: r->tiles )
            {
                assert( t->type != map_tile_t::WALL );
                assert( t->GetOwner() == ref );

                for ( ref_tile_region_t ref0: gen.regions )
                {
                    auto r0 = ref0.lock();

                    if ( r0 == r )
                    {
                        continue;
                    }

                    for ( const map_tile_t* t0: r0->tiles )
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

uint32_t bounds_region_t::count = 0;

bounds_region_t::bounds_region_t( void )
    : id( count++ )
{
}

bounds_region_t::bounds_region_t( const bounds_region_t& c )
    : id( c.id ),
      tiles( c.tiles ),
      region( c.region )
{
}

bool operator == ( const bounds_region_t& a, const bounds_region_t& b )
{
    return a.region == b.region;
}

bool operator != ( const bounds_region_t& a, const bounds_region_t& b )
{
    return !( a == b );
}

//-------------------------------------------------------------------------------------------------------
// tile_t
//-------------------------------------------------------------------------------------------------------

map_tile_t::map_tile_t( void )
    : entity_t( entity_t::BODY_DEPENDENT ),
      type( map_tile_t::EMPTY ),
	  x( 0 ), z( 0 ),
      halfSpaceIndex( -1 )
{
}

void map_tile_t::Set( const glm::mat4& transform )
{
    body.reset( new body_t );
    bounds.reset( new bounding_box_t );

    switch ( type )
    {
        case map_tile_t::BILLBOARD:
            depType = entity_t::BODY_DEPENDENT;
            body->SetFromTransform( transform );
            body->SetMass( 100.0f );
            Sync();
            break;
        default:
            if ( type == map_tile_t::WALL )
            {
                body->SetMass( 10.0f );
            }

            depType = entity_t::BOUNDS_DEPENDENT;
            assert( bounds->type == BOUNDS_PRIM_BOX );
            GetBoundsAsBox()->transform = transform;
            Sync();
            break;
    }
}

//-------------------------------------------------------------------------------------------------------
// tile_generator_t
//-------------------------------------------------------------------------------------------------------

tile_generator_t::tile_generator_t( collision_provider_t& collision_ )
    : collision( collision_ )
{
	tiles.resize( GRID_SIZE * GRID_SIZE );

	for ( uint32_t pass = 0; pass < GEN_PASS_COUNT; ++pass )
	{
		for ( uint32_t z = 0; z < GRID_SIZE; ++z )
		{
			for ( uint32_t x = 0; x < GRID_SIZE; ++x )
			{
                int32_t index = TileIndex( x, z );

                tiles[ index ].x = x;
                tiles[ index ].z = z;

                SetTile( tiles[ index ], pass );
			}
		}
	}

    // Iterate through each wall; any adjacent
    // tiles which aren't also walls will allow
    // for this wall to have a half-space
    // for the corresponding face.
    for ( map_tile_t* wall: walls )
    {
        collision_face_table_t halfSpaces;
        halfSpaces.fill( -1 );

        int32_t left = TileModIndex( wall->x - 1, wall->z );
        int32_t forward = TileModIndex( wall->x, wall->z - 1 );
        int32_t right = TileModIndex( wall->x + 1, wall->z );
        int32_t back = TileModIndex( wall->x, wall->z + 1 );

        bool hasHalfSpace = false;

        if ( tiles[ left ].type != map_tile_t::WALL && ( wall->x - 1 ) >= 0 )
        {
            halfSpaces[ COLLISION_FACE_LEFT ] = ( int32_t )collision.GenHalfSpace( *( wall->GetBoundsAsBox() ), COLLISION_FACE_LEFT );
            hasHalfSpace = true;
        }

        if ( tiles[ forward ].type != map_tile_t::WALL && ( wall->z - 1 ) >= 0 )
        {
            halfSpaces[ COLLISION_FACE_FORWARD ] = ( int32_t )collision.GenHalfSpace( *( wall->GetBoundsAsBox() ), COLLISION_FACE_FORWARD );
            hasHalfSpace = true;
        }

        if ( tiles[ right ].type != map_tile_t::WALL && ( wall->x + 1 ) < GRID_SIZE )
        {
            halfSpaces[ COLLISION_FACE_RIGHT ] = ( int32_t )collision.GenHalfSpace( *( wall->GetBoundsAsBox() ), COLLISION_FACE_RIGHT );
            hasHalfSpace = true;
        }

        if ( tiles[ back ].type != map_tile_t::WALL && ( wall->z + 1 ) < GRID_SIZE )
        {
            halfSpaces[ COLLISION_FACE_BACK ] = ( int32_t )collision.GenHalfSpace( *( wall->GetBoundsAsBox() ), COLLISION_FACE_BACK );
            hasHalfSpace = true;
        }

        if ( hasHalfSpace )
        {
            wall->halfSpaceIndex = ( int32_t ) collision.halfSpaceTable.size();
            collision.halfSpaceTable.push_back( std::move( halfSpaces ) );
        }
    }

    // Accumulate a list of tiles with halfSpaces, so we know
    // how to generate our regions for the map
    std::vector< const map_tile_t* > halfSpaceTiles;
    for ( const map_tile_t& tile: tiles )
    {
        if ( tile.halfSpaceIndex >= 0 )
        {
            halfSpaceTiles.push_back( &tile );
        }
    }

    // Generate regions

    // Fill basic regions
    for ( const map_tile_t* hst: halfSpaceTiles )
    {
        bool res = FindRegions( hst );
        assert( res );
    }

    // First pass
    FindAdjacentRegions();

    {
        region_merge_predicates_t predicates;
        const uint32_t TILE_LIMIT = uint32_t( float( TABLE_SIZE ) * 0.15f );

        predicates.entry = [ TILE_LIMIT ]( shared_tile_region_t& m ) -> bool
        { return m->tiles.size() <= TILE_LIMIT; };

        // No actual predicate necessary for our first merge pass
        predicates.adjacent = []( shared_tile_region_t& m, shared_tile_region_t& adj ) -> bool
        {
            UNUSEDPARAM( m );
            UNUSEDPARAM( adj );
            return true;
        };

        // Regions by default are very small, so we need to merge them together
        MergeRegions( predicates, 6 );
    }

    // Merging kills all original regions (mostly), so we need to do this a second time
    FindAdjacentRegions();

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
            return ( adj->tiles.size() < ADJ_TILE_LIMIT && m->tiles.size() < M_TILE_LIMIT );
        };

        MergeRegions( predicates, 4 );
    }

    FindAdjacentRegions();

    PurgeDefunctRegions();

    using transfer_predicate_t = std::function< bool( map_tile_t* ) >;
    transfer_predicate_t LDoSwapIf = []( map_tile_t* t ) -> bool
    {
        bool needsSwap = !t->HasOwner();

        if ( needsSwap )
        {
            t->type = map_tile_t::WALL;
        }

        return needsSwap;
    };

    Vector_TransferIf< map_tile_t*, transfer_predicate_t >( walls, freeSpace, LDoSwapIf );
    Vector_TransferIf< map_tile_t*, transfer_predicate_t >( walls, billboards, LDoSwapIf );

    // Compute tile boundries for every region
    for ( ref_tile_region_t region: regions )
    {
        auto r = region.lock();
        assert( r );

        // NOTE: it's important to remember that, since we're searching [ x - 1, x + 1 ] and [ z - 1, z + 1 ]
        // entirely, that tiles which are only diagnolly adjacent to either a wall or a different region
        // will also be added. This may or may not be desired.
        IterateTileListRange( r->tiles, -1, 1, [ this, &r ]( const map_tile_t& tile, int32_t x, int32_t z )
        {
            int32_t index = TileModIndex( x, z );
            auto r0 = tiles[ index ].owner.lock();
            const map_tile_t* t = &tiles[ index ];

            if ( r0 != r )
            {
                // if !r0, then we're next to a wall;
                // the regionTable isn't aware of wall tiles,
                // and thus has a nullptr for any index mapped to an area where a wall would be
                if ( r0 )
                {
                    assert( t->type != map_tile_t::WALL );

                    bounds_region_t* boundsRegion = r->FindAdjacentOwner( t );
                    assert( boundsRegion );

                    boundsRegion->tiles.push_back( &tile );
                }
                else
                {
                    assert( t->type == map_tile_t::WALL );
                    r->wallTiles.push_back( &tile );
                }
            }
        });
    }

    // Find an origin...
    // Do an integration...
    for ( ref_tile_region_t region: regions )
    {
        bool success = ComputeOrigin( region, tiles );
        assert( success );

        if ( success )
        {
            auto r = region.lock();

            bounding_box_t b = ComputeBoundsFromRegion( r.get() );

            r->boundsVolume.reset( new quad_hierarchy_t( std::move( b ), 5, r->GetEntityList() ) );
        }
    }

    assert( GeneratorTest( *this ) );
}

namespace {
    struct region_swap_entry_t
    {
        const map_tile_t* t;
        int32_t index;
    };
}

bool tile_generator_t::FindRegions( const map_tile_t* tile )
{
    if ( !tile )
    {
        return false;
    }

    if ( tile->halfSpaceIndex < 0 )
    {
        return false;
    }

    // Use the half-space index for the wall tile
    // by iterating through each half-space normal and finding tiles which lie in its
    // pathway until it hits a wall. Once it hits a wall,
    // we peace out and move onto the next normal. The result is
    // some regions which need to be "remixed" down the road.

    const collision_face_table_t& hst = collision.halfSpaceTable[ tile->halfSpaceIndex ];

    shared_tile_region_t regionPtr( new tile_region_t( tile ) );

    auto LSetTableEntry = []( shared_tile_region_t& region, const map_tile_t& t )
    {
        t.SetOwner( region );
        region->tiles.push_back( &t );
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
        const glm::vec3& e1 = collision.halfSpaces[ hst[ i ] ].extents[ 2 ] * 10.0f;
        const glm::vec3& e2 = collision.halfSpaces[ hst[ j ] ].extents[ 2 ] * 10.0f;

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

        const map_tile_t* zWall = nullptr;
        const map_tile_t* xWall = nullptr;

        // if zp or xp is -1 then we need to move towards the beginning
        // of the grid for that axis, as opposed to towards the end.
        int32_t endZ = ( zp > GRID_START )? GRID_END: GRID_START - 1;
        int32_t endX = ( xp > GRID_START )? GRID_END: GRID_START - 1;

        // Move in that direction; a wall which is found in the X or Z direction marks the end,
        // of the X or Z value, respectively. For example, if x starts at 0, but has a wall at x = 5, z
        // starts at 0 and z has a wall at z = 5, then we have a region entirely within the area covered
        // by x = 5 and z = 5, from x = 0 and z = 0.
        for ( int32_t z = tile->z; z != endZ; z += zp )
        {
            const map_tile_t& ztest = tiles[ TileIndex( tile->x, z ) ];

            if ( ztest.type == map_tile_t::WALL && !zWall && &ztest != tile )
            {
                zWall = &ztest;
            }

            for ( int32_t x = tile->x; x != endX; x += xp )
            {
                const map_tile_t& xtest = tiles[ TileIndex( x, tile->z ) ];

                if ( xtest.type == map_tile_t::WALL && !xWall && &xtest != tile )
                {
                    xWall = &xtest;
                }

                int32_t index = TileIndex( x, z );

                const map_tile_t& t = tiles[ index ];

                if ( &t == xWall )
                {
                    break;
                }

                if ( &t == tile || t.type == map_tile_t::WALL )
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
                tile_region_t* regionToSwap = nullptr;
                {
                    auto pRegion = tiles[ index ].owner.lock();
                    if ( pRegion )
                    {
                        regionToSwap = pRegion.get();
                        for ( const map_tile_t* t0: pRegion->tiles )
                        {
                            glm::vec2 o0( pRegion->origin->x, pRegion->origin->z );
                            glm::vec2 o1( regionPtr->origin->x, regionPtr->origin->z );
                            glm::vec2 p( ( float ) t0->x, ( float ) t0->z );

                            if ( glm::distance( o1, p ) < glm::distance( o0, p ) )
                            {
                                regionSwaps.push_back( { t0, TileModIndex( t0->x, t0->z ) } );
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
                    Vector_RemovePtr< const map_tile_t* >( regionToSwap->tiles, e.t );
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

    regions.push_back( std::shared_ptr< tile_region_t >( regionPtr ) );

    return true;
}

void tile_generator_t::FindAdjacentRegions( void )
{
    // Clear out any prior adjacent regions
    for ( ref_tile_region_t ref: regions )
    {
        auto p = ref.lock();
        if ( p )
        {
            p->adjacent.clear();
        }
    }

    // Find all regions which lie adjacent to each individual region
    for ( ref_tile_region_t region: regions )
    {
        bounds_region_list_t unique;
        auto r0 = region.lock();
        assert( r0 );

        // For every tile in r0, look for adjacent tiles which reside in a different region than r0.
        IterateTileListRange( r0->tiles, -1, 1, [ this, &r0, &unique ]( const map_tile_t& t, int32_t x, int32_t z )
        {
            UNUSEDPARAM( t );

            const ref_tile_region_t& rr = tiles[ TileIndex( x, z ) ].GetOwner();

            bool addIt = false;
            {
                auto r1 = rr.lock();
                addIt = r1 && ( r1 != r0 ) && !r1->tiles.empty();
            }

            if ( addIt )
            {
                bounds_region_t br;
                br.region = rr;

                unique.push_back( br );
            }
        });

        // Add the adjacent regions to r0's list
        for ( const bounds_region_t& r: unique )
        {
            assert( r.region != region );
            if ( !Vector_Contains( r0->adjacent, r ) )
            {
                r0->adjacent.push_back( r );
            }
        }
    }
}

void tile_generator_t::PurgeDefunctRegions( void )
{
    // Cleanup now defunct regions which have been merged.
    for ( auto r = regions.begin(); r != regions.end(); )
    {
        ref_tile_region_t wp = *r;

        auto p = wp.lock();

        if ( ( p && ( p->ShouldDestroy() || p->tiles.size() < MIN_REGION_SIZE ) ) || !p )
        {
            if ( p )
            {
                for ( const map_tile_t* t: p->tiles )
                {
                    t->owner.reset();
                }
            }

            r = regions.erase( r );

            PurgeFromAdjacent( regions, p );
        }
        else
        {
            ++r;
        }
    }
}

void tile_generator_t::MergeRegions( const region_merge_predicates_t& predicates, const uint32_t maxDepth )
{
    std::vector< shared_tile_region_t > merged;

    for ( ref_tile_region_t region: regions )
    {
        shared_tile_region_t merge( new tile_region_t() );

        Merge( merge, region, predicates, 0, maxDepth );

        if ( merge->tiles.empty() )
        {
            continue;
        }

        merged.push_back( std::move( merge ) );
    }

    PurgeDefunctRegions();

    regions.insert( regions.end(), merged.begin(), merged.end() );
}

int32_t tile_generator_t::RangeCount( const map_tile_t& t, int32_t startOffset, int32_t endOffset )
{
    int32_t count = 0;

    IterateTileRange( t, startOffset, endOffset, [ this, &count ]( const map_tile_t& tile, int32_t x, int32_t z )
    {
        UNUSEDPARAM( tile );

        if ( tiles[ TileIndex( x, z ) ].type != map_tile_t::EMPTY )
        {
            count++;
        }
    });

	return count;
}

void tile_generator_t::SetTile( map_tile_t& tile, int32_t pass )
{
    bool isWall;

	if ( pass == 0 )
	{
		isWall = wallDet( randEngine ) <= 40u;
	}
	else
	{
        isWall = RangeCount( tile, -1, 1 ) >= 5 || predicates[ pass - 1 ]( RangeCount( tile, -2, 2 ) );
	}

    tile.bounds.reset();
    tile.type = map_tile_t::EMPTY;

	// Multiply x and z values by 2 to accomodate for bounds scaling on the axis
	if ( isWall )
	{
        tile.type = map_tile_t::WALL;
		if ( pass == GEN_PASS_COUNT - 1 )
		{
            walls.push_back( &tile );
		}
	}
	else
	{
		// There is a 5% chance that billboards will be added
		if ( wallDet( randEngine ) <= 5u )
		{
            tile.type = map_tile_t::BILLBOARD;
			if ( pass == GEN_PASS_COUNT - 1 )
			{
                billboards.push_back( &tile );
			}

		}
		else
		{
			if ( pass == GEN_PASS_COUNT - 1 )
			{
                freeSpace.push_back( &tile );
			}
		}
	}

    if ( pass == GEN_PASS_COUNT - 1 )
	{
        tile.size = 1.0f;

        glm::mat4 t( glm::translate( glm::mat4( 1.0f ),
                                     glm::vec3( TRANSLATE_STRIDE * tile.x, 0.0f, TRANSLATE_STRIDE * tile.z ) ) );

        tile.Set( t * tile.GenScaleTransform() );
	}
}

bool tile_generator_t::CollidesWall( glm::vec3& normal, const map_tile_t& t,
                                const bounding_box_t& bounds,
                                half_space_t& outHalfSpace )
{
    if ( t.halfSpaceIndex < 0 )
    {
        return false;
    }

    const collision_face_table_t& halfSpaceFaces = collision.halfSpaceTable[ t.halfSpaceIndex ];

    for ( uint32_t i = 0; i < halfSpaceFaces.size(); ++i )
    {
        if ( halfSpaceFaces[ i ] >= 0 )
        {
            const half_space_t& hs = collision.halfSpaces[ halfSpaceFaces[ i ] ];

            if ( bounds.IntersectsHalfSpace( normal, hs ) )
            {
                if ( glm::length( normal ) < 1.0f )
                {
                    normal = glm::normalize( normal );
                }

                outHalfSpace = hs;
                return true;
            }

            if ( immDrawer )
            {
                hs.Draw( *immDrawer );
            }
        }
    }


	return false;
}

void tile_generator_t::GetEntities( map_tile_list_t& outBillboards,
                               map_tile_list_t& outWalls,
                               map_tile_list_t& outFreeSpace,
                               const frustum_t& frustum,
                               const view_params_t& viewParams )
{
	outBillboards.clear();
	outWalls.clear();
	outFreeSpace.clear();

	int32_t centerX = ( int32_t )( viewParams.origin.x * 0.5f );
	int32_t centerZ = ( int32_t )( viewParams.origin.z * 0.5f );

	if ( centerX < 0 || centerZ < 0 )
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

			int32_t index = TileModIndex( x, z );

            // cull frustum, insert into appropriate type, etc.
            if ( !frustum.IntersectsBox( *( tiles[ index ].GetBoundsAsBox() ) ) )
			{
				continue;
			}

			switch ( tiles[ index ].type )
			{
                case map_tile_t::BILLBOARD:
					outBillboards.push_back( &tiles[ index ] );
					break;
                case map_tile_t::WALL:
					outWalls.push_back( &tiles[ index ] );
					break;
                case map_tile_t::EMPTY:
					outFreeSpace.push_back( &tiles[ index ] );
					break;
			}
		}
	}

    // TODO: do view-space depth sort on each vector
}

//-------------------------------------------------------------------------------------------------------
// tile_region_t
//-------------------------------------------------------------------------------------------------------

tile_region_t::tile_region_t( const map_tile_t* origin_ )
    : destroy( false ),
      origin( origin_ ),
      color( RandomColor() ),
      boundsVolume( nullptr )
{
}

void tile_region_t::Draw( const pipeline_t& pl, const view_params_t& vp )
{
    UNUSEDPARAM( pl );
    UNUSEDPARAM( vp );
}

void tile_region_t::Destroy( void ) const
{
    destroy = true;
}

bool tile_region_t::ShouldDestroy( void ) const
{
    return destroy;
}

bounds_region_t* tile_region_t::FindAdjacentOwner( const map_tile_t* t )
{
    for ( auto i = adjacent.begin(); i != adjacent.end(); ++i )
    {
        if ( Vector_Contains< const map_tile_t* >( ( *i ).region.lock()->tiles, t ) )
        {
            return &( *i );
        }
    }

    return nullptr;
}

quad_hierarchy_t::entity_list_t tile_region_t::GetEntityList( void ) const
{
    quad_hierarchy_t::entity_list_t entities;
    entities.resize( tiles.size() );

    for ( uint32_t i = 0; i < entities.size(); ++i )
    {
        entities[ i ] = ( const entity_t* ) tiles[ i ];
    }

    return std::move( entities );
}


void tile_region_t::Update( void )
{
    if ( !boundsVolume )
    {
        return;
    }

    boundsVolume->Update( std::move( GetEntityList() ) );
}
