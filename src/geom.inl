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
// transform
//-------------------------------------------------------------------------------------------------------

INLINE transform_stack::transform_stack( void )
	: mScale( 0.0f ), mRotation( 0.0f ),
	  mTranslation( 0.0f ),
	  mTop( 1.0f )
{
}

INLINE void transform_stack::push( void )
{
	mMatStack.push( mTop );
}

INLINE void transform_stack::pop( void )
{
	mMatStack.pop();
}

INLINE const glm::mat4& transform_stack::peek( void ) const
{
	return mTop;
}

INLINE void transform_stack::scale( const glm::vec3& s )
{
	mScale = s;
}

INLINE void transform_stack::rotation( const glm::vec3& r )
{
	mRotation = r;
}

INLINE void transform_stack::translation( const glm::vec3& t )
{
	mTranslation = t;
}

INLINE void transform_stack::apply_scale( void )
{
	mTop *= glm::scale( glm::mat4( 1.0f ), mScale );
}

INLINE void transform_stack::apply_rotation( void )
{
	glm::mat4 r( 1.0f );
	rotate_matrix_xyz( r, mRotation );
	mTop *= r;
}

INLINE void transform_stack::apply_translation( void )
{
	mTop *= glm::translate( glm::mat4( 1.0f ), mTranslation );
}

INLINE void transform_stack::apply_scale( const glm::vec3& s )
{
	mScale = s;
	apply_scale();
}

INLINE void transform_stack::apply_rotation( const glm::vec3& r )
{
	mRotation = r;
	apply_rotation();
}

INLINE void transform_stack::apply_translation( const glm::vec3& t )
{
	mTranslation = t;
	apply_translation();
}

INLINE glm::mat3 transform_stack::scale( void ) const
{
	return std::move( glm::mat3( glm::scale( glm::mat4( 1.0f ), mScale ) ) );
}

INLINE glm::mat3 transform_stack::rotation( void ) const
{
	glm::mat4 r;
	rotate_matrix_xyz( r, mRotation );
	return std::move( glm::mat3( r ) );
}

INLINE glm::mat4 transform_stack::translation( void ) const
{
	return std::move( glm::translate( glm::mat4( 1.0f ), mTranslation ) );
}

//-------------------------------------------------------------------------------------------------------
// bounding_box_t
//-------------------------------------------------------------------------------------------------------

INLINE glm::vec3 obb::corner_identity( corner_type index ) const
{
	return glm::vec3(
		( ( int32_t ) index & 1 ) ? 1.0f : -1.0f,
		( ( int32_t ) index & 2 ) ? 1.0f : -1.0f,
		( ( int32_t ) index & 4 ) ? 1.0f : -1.0f
	);
}

INLINE bool	obb::range_x( const glm::vec3& v ) const
{
	glm::vec3 max( corner( CORNER_MAX ) );
	glm::vec3 min( corner( CORNER_MIN ) );

	return v.x <= max.x && v.x >= min.x;
}

INLINE bool obb::range_y( const glm::vec3& v ) const
{
	glm::vec3 max( corner( CORNER_MAX ) );
	glm::vec3 min( corner( CORNER_MIN ) );

	return v.y <= max.y && v.y >= min.y;
}

INLINE bool obb::range_z( const glm::vec3& v ) const
{
#ifdef AABB_MAX_Z_LESS_THAN_MIN_Z
	return v.z >= GetCorner( CORNER_MAX ).z && v.z <= GetCorner( CORNER_MIN ).z;
#else
	glm::vec3 max( corner( CORNER_MAX ) );
	glm::vec3 min( corner( CORNER_MIN ) );

	return v.z <= max.z && v.z >= min.z;
#endif
}

INLINE void obb::points( pointlist3D_t& points ) const
{
	points[ 0 ] = corner( ( corner_type ) 0 );
	points[ 1 ] = corner( ( corner_type ) 1 );
	points[ 2 ] = corner( ( corner_type ) 2 );
	points[ 3 ] = corner( ( corner_type ) 3 );
	points[ 4 ] = corner( ( corner_type ) 4 );
	points[ 5 ] = corner( ( corner_type ) 5 );
	points[ 6 ] = corner( ( corner_type ) 6 );
	points[ 7 ] = corner( ( corner_type ) 7 );
}

INLINE const glm::mat4& obb::axes( void ) const
{
	return mAxes;
}

INLINE void obb::center( const glm::vec3& position )
{
	mAxes[ 3 ] = glm::vec4( position, 1.0f );
}

INLINE void obb::orientation( const glm::mat3& orient )
{
	mAxes[ 0 ] = glm::vec4( orient[ 0 ], 0.0f );
	mAxes[ 1 ] = glm::vec4( orient[ 1 ], 0.0f );
	mAxes[ 2 ] = glm::vec4( orient[ 2 ], 0.0f );
}

INLINE const glm::vec4& obb::operator[]( uint32_t i ) const
{
	assert( i < 4 );

	return mAxes[ i ];
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

