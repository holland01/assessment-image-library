#include "map.h"
#include "view.h"
#include <iostream>
#include <random>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/projection.hpp>

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
}

//-------------------------------------------------------------------------------------------------------
// tile_t
//-------------------------------------------------------------------------------------------------------

tile_t::tile_t( void )
    : entity_t( entity_t::BODY_DEPENDENT ),
      type( tile_t::EMPTY ),
	  x( 0 ), z( 0 ),
      halfSpaceIndex( -1 )
{
}

void tile_t::Set( const glm::mat4& transform )
{
    body.reset( new body_t );
    bounds.reset( new bounding_box_t );

    switch ( type )
    {
        case tile_t::BILLBOARD:
            depType = entity_t::BODY_DEPENDENT;
            body->SetFromTransform( transform );
            body->SetMass( 100.0f );
            Sync();
            break;
        default:
            if ( type == tile_t::WALL )
            {
                body->SetMass( 10.0f );
            }

            depType = entity_t::BOUNDS_DEPENDENT;
            bounds->SetTransform( transform );
            Sync();
            break;
    }
}

//-------------------------------------------------------------------------------------------------------
// tile_generator_t
//-------------------------------------------------------------------------------------------------------

tile_generator_t::tile_generator_t( void )
{
	billTexture.mipmap = true;
	billTexture.LoadFromFile( "asset/mooninite.png" );
	billTexture.Load2D();

	tiles.resize( GRID_SIZE * GRID_SIZE );

	// used to compute half-spaces
	std::vector< tile_t* > mutWalls;

	for ( uint32_t pass = 0; pass < GEN_PASS_COUNT; ++pass )
	{
		for ( uint32_t z = 0; z < GRID_SIZE; ++z )
		{
			for ( uint32_t x = 0; x < GRID_SIZE; ++x )
			{
				SetTile( pass, x, z, mutWalls );
			}
		}
	}

	std::array< glm::vec3, NUM_FACES > halfSpaceNormals =
	{{
		glm::vec3( -1.0f, 0.0f, 0.0f ),
		glm::vec3( 0.0f, 0.0f, -1.0f ),
		glm::vec3( 1.0f, 0.0f, 0.0f ),
		glm::vec3( 0.0f, 0.0f, 1.0f )
	}};

	auto LAddHalfSpace = [ this, &halfSpaceNormals ]( const tile_t& t, int32_t& index, faceIndex_t face )
	{
		index = halfSpaces.size();
		half_space_t hs = GenHalfSpace( t, halfSpaceNormals[ face ] );
		halfSpaces.push_back( std::move( hs ) );
	};

	// Iterate through each wall; any adjacent
	// tiles which aren't also walls will allow
	// for this wall to have a half-space
	// for the corresponding face.
	for ( tile_t* wall: mutWalls )
	{
		half_space_table_t halfSpaces;
		halfSpaces.fill( -1 );

		int32_t left = TileModIndex( wall->x - 1, wall->z );
		int32_t forward = TileModIndex( wall->x, wall->z - 1 );
		int32_t right = TileModIndex( wall->x + 1, wall->z );
		int32_t back = TileModIndex( wall->x, wall->z + 1 );

		bool hasHalfSpace = false;

		if ( tiles[ left ].type != tile_t::WALL && ( wall->x - 1 ) >= 0 )
		{
			LAddHalfSpace( *wall, halfSpaces[ FACE_LEFT ], FACE_LEFT );
			hasHalfSpace = true;
		}

		if ( tiles[ forward ].type != tile_t::WALL && ( wall->z - 1 ) >= 0 )
		{
			LAddHalfSpace( *wall, halfSpaces[ FACE_FORWARD ], FACE_FORWARD );
			hasHalfSpace = true;
		}

		if ( tiles[ right ].type != tile_t::WALL && ( wall->x + 1 ) < GRID_SIZE )
		{
			LAddHalfSpace( *wall, halfSpaces[ FACE_RIGHT ], FACE_RIGHT );
			hasHalfSpace = true;
		}

		if ( tiles[ back ].type != tile_t::WALL && ( wall->z + 1 ) < GRID_SIZE )
		{
			LAddHalfSpace( *wall, halfSpaces[ FACE_BACK ], FACE_BACK );
			hasHalfSpace = true;
		}

		if ( hasHalfSpace )
		{
			wall->halfSpaceIndex = halfSpaceTable.size();
			halfSpaceTable.push_back( std::move( halfSpaces ) );
		}
	}

    std::array< uint8_t, GRID_SIZE * GRID_SIZE > regioned;
    regioned.fill( 0 );

    tile_region_t r;
    r.color = RandomColor();

    auto LResetRegion = [ & ]( void )
    {
        regions.push_back( r );
        r.tiles.clear();
        r.color = RandomColor();
    };

    const uint32_t Z_COMP = 0;
    //const uint32_t X_COMP = 1;

    using comp_index_fn_t = std::function< uint32_t( uint32_t k, uint32_t x, uint32_t z ) >;

    std::array< comp_index_fn_t, 2 > indexComputeTable =
    {{
        []( uint32_t k, uint32_t x, uint32_t z ) -> uint32_t
        {
            UNUSEDPARAM( z );
            return k * GRID_SIZE + x;
        },

        []( uint32_t k, uint32_t x, uint32_t z ) -> uint32_t
        {
            UNUSEDPARAM( x );
            return z * GRID_SIZE + k;
        }
    }};

    auto LMatchColumn = [ & ]( const tile_t& t,
            uint32_t x, uint32_t z, uint32_t base,
            uint32_t indexComp,
            faceIndex_t startFace,
            faceIndex_t endFace )
    {
        const half_space_table_t& hst = halfSpaceTable[ t.halfSpaceIndex ];

        if ( hst[ startFace ] < 0 )
        {
            return;
        }

        const half_space_t& hs = halfSpaces[ startFace ];

        glm::vec3 normal( glm::normalize( hs.extents[ 2 ] ) );

        const glm::vec3& match = halfSpaceNormals[ startFace ];

        float d = glm::dot( normal, match );

        if ( d == 1.0f )
        {
            for ( uint32_t k = base + 1; k < GRID_SIZE; ++k )
            {
                uint32_t index = indexComputeTable[ indexComp ]( k, x, z );

                const tile_t& t0 = tiles[ index ];

                if ( t0.type == tile_t::WALL )
                {
                    // This should not happen, see below comment...
                    assert( t0.halfSpaceIndex >= 0 );
                    assert( halfSpaceTable[ t0.halfSpaceIndex ][ endFace ] >= 0 );


                    // No need to check for a half-space index, here: we only can only
                    // get to this point if the previous checked half-space ( in the scope outside of this loop )
                    // has a normal which faces +z. This is only possible if there is a tile in front of its +zface
                    // which is not a wall.
                    /*{
                        const half_space_table_t& hst0 = halfSpaceTable[ t0.halfSpaceIndex ];
                        for ( int32_t i: hst0 )
                        {
                            if ( i < 0 )
                            {
                                continue;
                            }

                            if ( glm::dot( normal, glm::normalize( halfSpaces[ i ].extents[ 2 ] ) ) == -1.0f )
                            {
                                break;
                            }
                        }
                    }*/

                    // We've found a wall, here: since we're moving downward,
                    // there's no way a tile could have a valid half space after this, regardless of what's current. So, we're done.
                    break;
                }

                regioned[ index ] = 1;
                r.tiles.push_back( &t0 );
            }
        }
    };

    // Find regions
    for ( uint32_t x = 0; x < GRID_SIZE; ++x )
    {
        if ( ( ( x + 1 ) % 5 ) == 0 )
        {
            LResetRegion();
        }

        for ( uint32_t z = 0; z < GRID_SIZE; ++z )
        {
            uint32_t zSave = z;

            if ( regioned[ z * GRID_SIZE + x ] )
            {
                continue;
            }

            const tile_t* t = &tiles[ z * GRID_SIZE + x ];

            // Try once to find a wall with a halfspace if we don't have anything yet.
            if ( t->halfSpaceIndex < 0 )
            {
                z++;
                while ( z < GRID_SIZE )
                {
                    t = &tiles[ z * GRID_SIZE + x ];

                    if ( t->halfSpaceIndex >= 0 )
                    {
                        LMatchColumn( *t, x, z, z, Z_COMP, FACE_BACK, FACE_FORWARD );

                        break;
                    }

                    z++;
                }
            }

            z = zSave;
        }
    }
}

uint32_t tile_generator_t::RangeCount( uint32_t x, uint32_t z, uint32_t endOffset )
{
	if ( ( int32_t )x < 0 )
	{
		x = 0;
	}

	if ( ( int32_t )z < 0  )
	{
		z = 0;
	}

	uint32_t ex = ( x + endOffset );
	uint32_t ez = ( z + endOffset );

	uint32_t count = 0;
	for ( uint32_t iz = z; iz < ez; ++iz )
	{
		for ( uint32_t ix = x; ix < ex; ++ix )
		{
			uint32_t i = TileModIndex( ix, iz );

			if ( tiles[ i ].type != tile_t::EMPTY )
			{
				count++;
			}
		}
	}

	return count;
}

void tile_generator_t::SetTile( uint32_t pass, uint32_t x, uint32_t z, std::vector< tile_t* >& mutWalls )
{
	bool isWall;
	uint32_t center = TileIndex( x, z );

	if ( pass == 0 )
	{
		isWall = wallDet( randEngine ) <= 40u;
	}
	else
	{
		isWall = RangeCount( x - 1, z - 1, 3 ) >= 5 || predicates[ pass - 1 ]( RangeCount( x - 2, z - 2, 5 ) );
	}

    tiles[ center ].bounds.reset();
	tiles[ center ].type = tile_t::EMPTY;
	tiles[ center ].x = x;
	tiles[ center ].z = z;

	// Multiply x and z values by 2 to accomodate for bounds scaling on the axis
	if ( isWall )
	{
		tiles[ center ].type = tile_t::WALL;
		if ( pass == GEN_PASS_COUNT - 1 )
		{
			walls.push_back( &tiles[ center ] );
			mutWalls.push_back( &tiles[ center ] );
		}
	}
	else
	{
		// There is a 5% chance that billboards will be added
		if ( wallDet( randEngine ) <= 5u )
		{
			tiles[ center ].type = tile_t::BILLBOARD;
			if ( pass == GEN_PASS_COUNT - 1 )
			{
				billboards.push_back( &tiles[ center ] );
			}

		}
		else
		{
			if ( pass == GEN_PASS_COUNT - 1 )
			{
				freeSpace.push_back( &tiles[ center ] );
			}
		}
	}

    if ( pass == GEN_PASS_COUNT - 1 )
	{
        tiles[ center ].SetSize( ( tiles[ center ].type == tile_t::BILLBOARD )? 1.0f: 1.0f );

		glm::mat4 t( glm::translate( glm::mat4( 1.0f ), glm::vec3( 2.0f * x, 0.0f, 2.0f * z ) ) );

        tiles[ center ].Set( t * tiles[ center ].GenScaleTransform() );
	}
}

half_space_t tile_generator_t::GenHalfSpace( const tile_t& t, const glm::vec3& normal )
{
	using corner_t = bounding_box_t::corner_t;

    glm::vec3 upAxis( ( *t.bounds )[ 1 ] );
    glm::vec3 boundsOrigin( ( *t.bounds )[ 3 ] );
    glm::vec3 boundsSize( t.bounds->GetSize() );

	half_space_t hs;

	hs.extents[ 0 ] = std::move( glm::cross( normal, upAxis ) ) * boundsSize[ 0 ];
	hs.extents[ 1 ] = upAxis * boundsSize[ 1 ];
	hs.extents[ 2 ] = normal * 0.1f;

	glm::vec3 faceCenter( std::move( boundsOrigin + normal * boundsSize * 0.5f ) );

	// TODO: take into account ALL points
	std::array< corner_t, 4 > lowerPoints =
	{{
		bounding_box_t::CORNER_MIN,
		bounding_box_t::CORNER_NEAR_DOWN_RIGHT,
		bounding_box_t::CORNER_FAR_DOWN_RIGHT,
		bounding_box_t::CORNER_FAR_DOWN_LEFT
	}};

	for ( corner_t face: lowerPoints )
	{
        glm::vec3 point( t.bounds->GetCorner( ( corner_t ) face ) );
		glm::vec3 pointToCenter( faceCenter - point );

		// Not in same plane; move on
        if ( G_TripleProduct( pointToCenter, hs.extents[ 0 ], hs.extents[ 1 ] ) != 0.0f )
		{
			continue;
		}

		// Half space axes will be outside of the bounds; move on
        if ( !t.bounds->EnclosesPoint( point + hs.extents[ 0 ] ) || !t.bounds->EnclosesPoint( point + hs.extents[ 1 ] ) )
		{
			continue;
		}

		hs.origin = point;
		break;
	}

	hs.distance = glm::dot( hs.extents[ 2 ], hs.origin );

	return std::move( hs );
}

bool tile_generator_t::CollidesWall( glm::vec3& normal, const tile_t& t,
								const bounding_box_t& bounds,
								half_space_t& outHalfSpace )
{
	if ( t.halfSpaceIndex < 0 )
	{
		return false;
	}

	const half_space_table_t& halfSpaceFaces = halfSpaceTable[ t.halfSpaceIndex ];

    for ( uint32_t i = 0; i < halfSpaceFaces.size(); ++i )
	{
		if ( halfSpaceFaces[ i ] >= 0 )
		{	
			const half_space_t& hs = halfSpaces[ halfSpaceFaces[ i ] ];

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

void tile_generator_t::GetEntities( billboard_list_t& outBillboards,
                               wall_list_t& outWalls,
                               freespace_list_t& outFreeSpace,
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

			if ( !frustum.IntersectsBox( *tiles[ index ].bounds ) )
			{
				continue;
			}

			switch ( tiles[ index ].type )
			{
				case tile_t::BILLBOARD:
					outBillboards.push_back( &tiles[ index ] );
					break;
				case tile_t::WALL:
					outWalls.push_back( &tiles[ index ] );
					break;
				case tile_t::EMPTY:
					outFreeSpace.push_back( &tiles[ index ] );
					break;
			}
			// cull frustum, insert into appropriate type, etc.
		}
	}

	// do view-space depth sort on each vector
}

//-------------------------------------------------------------------------------------------------------
// tile_region_t
//-------------------------------------------------------------------------------------------------------

tile_region_t::tile_region_t( void )
    : color( 0.0f )
{
}

void tile_region_t::Draw( const pipeline_t& pl, const view_params_t& vp )
{
    UNUSEDPARAM( pl );
    UNUSEDPARAM( vp );
}

//-------------------------------------------------------------------------------------------------------
// quad_hierarchy_t
//
// | II.  | I. |
// | III. | IV. |
//----------------------------------------------------------------------------------------------------

namespace {
    const glm::vec3 PLANE_VEC( 1.0f, 0.0f, 1.0f );
    const glm::vec3 CONST_AXIS_VEC( 0.0f, 1.0f, 0.0f );
    const uint8_t CONST_AXIS = 1;
}

quad_hierarchy_t::node_t::node_t( uint32_t curDepth, const uint32_t maxDepth, bounding_box_t bounds_ )
    : bounds( std::move( bounds_ ) )
{
    if ( curDepth >= maxDepth - 1 )
    {
        return;
    }

    //glm::vec3 size( );
//    glm::vec3 quadSize( size * PLANE_VEC * 0.25f + CONST_AXIS_VEC * size[ CONST_AXIS ] );

    glm::vec3 quadSize( 0.5f, 1.0f, 0.5f );
    auto LMakeChild = [ & ]( uint8_t index, const glm::vec3& offset )
    {
        glm::mat4 s( glm::scale( glm::mat4( 1.0f ), quadSize ) );
        glm::mat4 t( glm::translate( glm::mat4( 1.0f ), offset ) );
        glm::mat4 m( t * s );
        children[ index ].reset( new node_t( curDepth + 1, maxDepth, std::move( bounding_box_t( m ) ) ) );
    };

    LMakeChild( 0, quadSize * PLANE_VEC );
    LMakeChild( 1, quadSize * glm::vec3( -1.0f, 0.0f, 1.0f ) );
    LMakeChild( 2, quadSize * glm::vec3( -1.0f , 0.0f, -1.0f ) );
    LMakeChild( 3, quadSize * glm::vec3( 1.0f, 0.0f, -1.0f ) );
}

void quad_hierarchy_t::node_t::Draw( const pipeline_t& pl, const view_params_t& vp, const glm::mat4& rootTransform ) const
{
    //const shader_program_t& singleColor = pl.programs.at( "single_color" );
    //const draw_buffer_t& coloredCube = pl.drawBuffers.at( "lined_cube" );

    glm::mat4 t( rootTransform * bounds.GetTransform() );

    /*
    {
        load_blend_t b( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

        singleColor.Bind();
        singleColor.LoadVec4( "color", glm::vec4( 0.3f, 0.3f, 0.3f, 1.0f ) );
        singleColor.LoadMat4( "modelToView", vp.transform * t );
        coloredCube.Render( singleColor );
        singleColor.Release();
    }*/

    for ( const node_t::ptr_t& n: children )
    {
        if ( n )
        {
            n->Draw( pl, vp, t );
        }
    }
}
