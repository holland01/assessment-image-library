//-------------------------------------------------------------------------------------------------------
// util
//-------------------------------------------------------------------------------------------------------

// rightAxis can be thought of as an initial axis with which the angle will originate from,
// dir is the terminating axis ending the angle, and backAxis is the axis which is orthogonal to the rightAxis
// such that the angle between the two is 270 degrees, counter-clockwise.
// All are assumed to be normalized
static INLINE glm::mat3 orient_by_direction( const glm::vec3& dir, const glm::vec3& rightAxis, const glm::vec3& backAxis )
{
	float rot = glm::acos( glm::dot( rightAxis, dir ) );

	if ( glm::dot( backAxis, dir ) > 0.0f )
	{
		rot = -rot;
	}

	return std::move( glm::mat3( glm::rotate( glm::mat4( 1.0f ), rot, glm::vec3( 0.0f, 1.0f, 0.0f ) ) ) );
}

static INLINE void rotate_matrix_xyz( glm::mat4& r, const glm::vec3& rotation )
{
	r = glm::rotate( glm::mat4( 1.0f ), rotation.x, glm::vec3( 1.0f, 0.0f, 0.0f ) );
	r = glm::rotate( r, rotation.y, glm::vec3( 0.0f, 1.0f, 0.0f ) );
	r = glm::rotate( r, rotation.z, glm::vec3( 0.0f, 0.0f, 1.0f ) );
}

using point_predicate_t = bool ( * )( float );

template < size_t N, point_predicate_t predicate >
static INLINE bool test_point_plane( const std::array< glm::vec3, N >& points, const plane& pln )
{
	for ( const glm::vec3& p: points )
	{
        float x = glm::dot( p, pln.mNormal ) - pln.mDistance;

		if ( ( *predicate )( x ) )
		{
			return true;
		}
	}

	return false;
}

static INLINE float triple_product( const glm::vec3& a, const glm::vec3& b, const glm::vec3& c )
{
	return glm::dot( a, glm::cross( b, c ) );
}

//-------------------------------------------------------------------------------------------------------
// bounds_primitive_t
//-------------------------------------------------------------------------------------------------------

INLINE primitive_lookup* bounds_primitive::to_lookup( void )
{
	assert( type == BOUNDS_PRIM_LOOKUP );
	return ( primitive_lookup* ) this;
}

INLINE const primitive_lookup* bounds_primitive::to_lookup( void ) const
{
	assert( type == BOUNDS_PRIM_LOOKUP );
	return ( const primitive_lookup* ) this;
}


INLINE obb* bounds_primitive::to_box( void )
{
	assert( type == BOUNDS_PRIM_BOX );
	return ( obb* ) this;
}

INLINE const obb* bounds_primitive::to_box( void ) const
{
	assert( type == BOUNDS_PRIM_BOX );
	return ( const obb* ) this;
}

INLINE halfspace* bounds_primitive::to_halfspace( void )
{
	assert( type == BOUNDS_PRIM_BOX );
	return ( halfspace* ) this;
}

