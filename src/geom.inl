
//-------------------------------------------------------------------------------------------------------
// util
//-------------------------------------------------------------------------------------------------------

// rightAxis can be thought of as an initial axis with which the angle will originate from,
// dir is the terminating axis ending the angle, and backAxis is the axis which is orthogonal to the rightAxis
// such that the angle between the two is 270 degrees, counter-clockwise.
// All are assumed to be normalized
static INLINE glm::mat3 G_OrientByDirection( const glm::vec3& dir, const glm::vec3& rightAxis, const glm::vec3& backAxis )
{
    float rot = glm::acos( glm::dot( rightAxis, dir ) );

    if ( glm::dot( backAxis, dir ) > 0.0f )
    {
        rot = -rot;
    }

    return std::move( glm::mat3( glm::rotate( glm::mat4( 1.0f ), rot, glm::vec3( 0.0f, 1.0f, 0.0f ) ) ) );
}

static INLINE void G_RotateMatrixXYZ( glm::mat4& r, const glm::vec3& rotation )
{
    r = glm::rotate( glm::mat4( 1.0f ), rotation.x, glm::vec3( 1.0f, 0.0f, 0.0f ) );
    r = glm::rotate( r, rotation.y, glm::vec3( 0.0f, 1.0f, 0.0f ) );
    r = glm::rotate( r, rotation.z, glm::vec3( 0.0f, 0.0f, 1.0f ) );
}

using point_predicate_t = bool ( * )( float );

template < size_t N, point_predicate_t predicate >
static INLINE bool G_PointPlaneTest( const std::array< glm::vec3, N >& points, const plane_t& plane )
{
    for ( const glm::vec3& p: points )
    {
        float x = glm::dot( p, plane.normal ) - plane.d;

        if ( ( *predicate )( x ) )
        {
            return true;
        }
    }

    return false;
}

static INLINE float G_TripleProduct( const glm::vec3& a, const glm::vec3& b, const glm::vec3& c )
{
    return glm::dot( a, glm::cross( b, c ) );
}

//-------------------------------------------------------------------------------------------------------
// transform_t
//-------------------------------------------------------------------------------------------------------

INLINE transform_t::transform_t( void )
    : scale( 0.0f ), rotation( 0.0f ),
      translation( 0.0f ),
      top( 1.0f )
{
}

INLINE void transform_t::PushTransform( void )
{
    matStack.push( top );
}

INLINE void transform_t::PopTransform( void )
{
    matStack.pop();
}

INLINE const glm::mat4& transform_t::PeekTransform( void ) const
{
    return top;
}

INLINE void transform_t::SetScale( const glm::vec3& s )
{
    scale = s;
}

INLINE void transform_t::SetRotation( const glm::vec3& r )
{
    rotation = r;
}

INLINE void transform_t::SetTranslation( const glm::vec3& t )
{
    translation = t;
}

INLINE void transform_t::ApplyScale( void )
{
    top *= glm::scale( glm::mat4( 1.0f ), scale );
}

INLINE void transform_t::ApplyRotation( void )
{
    glm::mat4 r( 1.0f );
    G_RotateMatrixXYZ( r, rotation );
    top *= r;
}

INLINE void transform_t::ApplyTranslation( void )
{
    top *= glm::translate( glm::mat4( 1.0f ), translation );
}

INLINE void transform_t::ApplyScale( const glm::vec3& s )
{
    scale = s;
    ApplyScale();
}

INLINE void transform_t::ApplyRotation( const glm::vec3& r )
{
    rotation = r;
    ApplyRotation();
}

INLINE void transform_t::ApplyTranslation( const glm::vec3& t )
{
    translation = t;
    ApplyTranslation();
}

INLINE glm::mat3 transform_t::GetScale( void ) const
{
    return std::move( glm::mat3( glm::scale( glm::mat4( 1.0f ), scale ) ) );
}

INLINE glm::mat3 transform_t::GetRotation3( void ) const
{
    glm::mat4 r;
    G_RotateMatrixXYZ( r, rotation );
    return std::move( glm::mat3( r ) );
}

INLINE glm::mat4 transform_t::GetTranslation( void ) const
{
    return std::move( glm::translate( glm::mat4( 1.0f ), translation ) );
}

//-------------------------------------------------------------------------------------------------------
// bounding_box_t
//-------------------------------------------------------------------------------------------------------

INLINE glm::vec3 bounding_box_t::GetCornerIdentity( corner_t index ) const
{
    return glm::vec3(
        ( ( int32_t ) index & 1 ) ? 1.0f : -1.0f,
        ( ( int32_t ) index & 2 ) ? 1.0f : -1.0f,
        ( ( int32_t ) index & 4 ) ? 1.0f : -1.0f
    );
}

INLINE bool	bounding_box_t::InXRange( const glm::vec3& v ) const
{

    return v.x <= GetCorner( CORNER_MAX ).x && v.x >= GetCorner( CORNER_MIN ).x;
}

INLINE bool bounding_box_t::InYRange( const glm::vec3& v ) const
{

    return v.y <= GetCorner( CORNER_MAX ).y && v.y >= GetCorner( CORNER_MIN ).y;
}

INLINE bool bounding_box_t::InZRange( const glm::vec3& v ) const
{
#ifdef AABB_MAX_Z_LESS_THAN_MIN_Z
    return v.z >= GetCorner( CORNER_MAX ).z && v.z <= GetCorner( CORNER_MIN ).z;
#else
    return v.z <= GetCorner( CORNER_MAX ).z && v.z >= GetCorner( CORNER_MIN ).z;
#endif
}

INLINE bool bounding_box_t::EnclosesPoint( const glm::vec3& v ) const
{
    return InXRange( v ) && InYRange( v ) && InZRange( v );
}

INLINE void bounding_box_t::GetPoints( std::array< glm::vec3, 8 >& points ) const
{
    points[ 0 ] = GetCorner( ( corner_t ) 0 );
    points[ 1 ] = GetCorner( ( corner_t ) 1 );
    points[ 2 ] = GetCorner( ( corner_t ) 2 );
    points[ 3 ] = GetCorner( ( corner_t ) 3 );
    points[ 4 ] = GetCorner( ( corner_t ) 4 );
    points[ 5 ] = GetCorner( ( corner_t ) 5 );
    points[ 6 ] = GetCorner( ( corner_t ) 6 );
    points[ 7 ] = GetCorner( ( corner_t ) 7 );
}

INLINE const glm::mat4& bounding_box_t::GetTransform( void ) const
{
    return transform;
}

INLINE void bounding_box_t::SetCenter( const glm::vec3& position )
{
    transform[ 3 ] = glm::vec4( position, 1.0f );
}

INLINE void bounding_box_t::SetOrientation( const glm::mat3& orient )
{
    transform[ 0 ] = glm::vec4( orient[ 0 ], 0.0f );
    transform[ 1 ] = glm::vec4( orient[ 1 ], 0.0f );
    transform[ 2 ] = glm::vec4( orient[ 2 ], 0.0f );
}

INLINE void bounding_box_t::SetTransform( const glm::mat4& t )
{
    transform = t;
}

INLINE const glm::vec4& bounding_box_t::operator[]( uint32_t i ) const
{
    assert( i < 4 );

    return transform[ i ];
}
