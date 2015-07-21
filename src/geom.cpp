#include "geom.h"
#include "view.h"
#include <array>
#include <string.h>

namespace geom {

//-------------------------------------------------------------------------------------------------------
// AABB
//-------------------------------------------------------------------------------------------------------
aabb_t::aabb_t( void )
{
    Empty();
}

aabb_t::aabb_t( const glm::vec3& max, const glm::vec3& min )
    : maxPoint( max ),
      minPoint( min )
{
}

aabb_t::aabb_t( const aabb_t& toCopy )
    : maxPoint( toCopy.maxPoint ),
      minPoint( toCopy.minPoint )
{
}

aabb_t::~aabb_t( void )
{
}

aabb_t& aabb_t::operator =( aabb_t toAssign )
{
    maxPoint = toAssign.maxPoint;
    minPoint = toAssign.minPoint;

    return *this;
}

void aabb_t::Add( const glm::vec3& p )
{
    if ( p.x < minPoint.x ) minPoint.x = p.x;
    if ( p.y < minPoint.y ) minPoint.y = p.y;
    if ( p.z < minPoint.z ) minPoint.z = p.z;

    if ( p.x > maxPoint.x ) maxPoint.x = p.x;
    if ( p.y > maxPoint.y ) maxPoint.y = p.x;
    if ( p.z > maxPoint.z ) maxPoint.z = p.z;
}

void aabb_t::Empty( void )
{
    const float pseudoInfinity = 1e37f;

#ifdef AABB_MAX_Z_LESS_THAN_MIN_Z
    maxPoint = glm::vec3( -pseudoInfinity, -pseudoInfinity, pseudoInfinity );
    minPoint = glm::vec3( pseudoInfinity, pseudoInfinity, -pseudoInfinity );
#else
    maxPoint = glm::vec3( -pseudoInfinity );
    minPoint = glm::vec3( pseudoInfinity );
#endif
}

void aabb_t::TransformTo( const aabb_t& box, const glm::mat4& transform )
{
    maxPoint = minPoint = glm::vec3( transform[ 3 ] );

    for ( int32_t i = 0; i < 3; ++i )
    {
        float px = transform[ i ][ 0 ];
        float py = transform[ i ][ 1 ];
        float pz = transform[ i ][ 2 ];

        // Scale each basis' respective coordinate
        // along its respective axis for our max/min coordinates.
        // If pn (where 'n' is { x | y | z }) is > 0, compute box's point
        // in parallel with this one's. Otherwise, compute this instances
        // points opposite to that of the box's, in an effort to 'shrink'
        // our current points.

        if ( px > 0.0f )
        {
            minPoint.x += box.minPoint.x * px;
            maxPoint.x += box.maxPoint.x * px;
        }
        else
        {
            minPoint.x += box.maxPoint.x * px;
            maxPoint.x += box.minPoint.x * px;
        }

        if ( py > 0.0f )
        {
            minPoint.y += box.minPoint.y * py;
            maxPoint.y += box.maxPoint.y * py;
        }
        else
        {
            minPoint.y += box.maxPoint.y * py;
            maxPoint.y += box.minPoint.y * py;
        }

        if ( pz > 0.0f )
        {
            minPoint.z += box.minPoint.z * pz;
            maxPoint.z += box.maxPoint.z * pz;
        }
        else
        {
            minPoint.z += box.maxPoint.z * pz;
            maxPoint.z += box.minPoint.z * pz;
        }
    }
}

glm::vec3 aabb_t::GetMaxRelativeToNormal( const glm::vec3 &normal ) const
{
    glm::vec3 p( minPoint.x, minPoint.y, minPoint.z );

    if ( normal.x >= 0 )
        p.x = maxPoint.x;

    if ( normal.y >= 0 )
        p.y = maxPoint.y;

    if ( normal.z >= 0 )
        p.z = maxPoint.z;

    return p;
}

glm::vec3 aabb_t::GetMinRelativeToNormal( const glm::vec3 &normal ) const
{
    glm::vec3 n( maxPoint.x, maxPoint.y, maxPoint.z );

    if ( normal.x >= 0 )
        n.x = minPoint.x;

    if ( normal.y >= 0 )
        n.y = minPoint.y;

    if ( normal.z >= 0 )
        n.z = minPoint.z;

    return n;
}


glm::vec3 aabb_t::Center( void ) const
{
    const glm::vec3& p = ( maxPoint + minPoint );

    return p / 2.0f;
}

glm::vec3 aabb_t::Size( void ) const
{
    return maxPoint - minPoint;
}

glm::vec3 aabb_t::Radius( void ) const
{
    return maxPoint - Center();
}

enum
{
    CORNER_MIN = 0,
    CORNER_NEAR_DOWN_RIGHT,
    CORNER_NEAR_UP_LEFT,
    CORNER_NEAR_UP_RIGHT,
    CORNER_FAR_DOWN_LEFT,
    CORNER_FAR_DOWN_RIGHT,
    CORNER_FAR_UP_LEFT,
    CORNER_MAX = 7
};

glm::vec3 aabb_t::Corner( int index ) const
{
    assert( index >= 0 );
    assert( index <= 7 );

    return glm::vec3(
        ( index & 1 ) ? maxPoint.x : minPoint.x,
        ( index & 2 ) ? maxPoint.y : minPoint.y,
        ( index & 4 ) ? maxPoint.z : minPoint.z
    );
}

bool aabb_t::IsEmpty( void ) const
{
    // Check to see if any of our
    // axes are inverted
    return ( maxPoint.x < minPoint.x )
        || ( maxPoint.y < minPoint.y )
#ifdef AABB_MAX_Z_LESS_THAN_MIN_Z
        || ( maxPoint.z < minPoint.z );
#else
        || ( maxPoint.z > minPoint.z );
#endif
}

bool aabb_t::InPointRange( float k ) const
{
    return ( maxPoint.x >= k && maxPoint.y >= k && maxPoint.z >= k )
        && ( minPoint.x <= k && minPoint.y <= k && minPoint.z <= k );
}

// Find the closest 3 faces
// Compute intersections;
// then make sure the ray will be within the bounds of the three faces;
float aabb_t::CalcIntersection( const glm::vec3& ray, const glm::vec3& origin ) const
{
    // Quick early out; 0 implies no scaling necessary
    if ( InXRange( origin ) && InYRange( origin ) && InZRange( origin ) )
    {
        return 0.0f;
    }

    //std::array< plane_t, 3 > faces;
    std::array< float, 3 > intersections;

    int32_t fcount = 0;
    for ( int32_t i = 0; i < 6; ++i )
    {
        if ( fcount == 3 )
        {
            break;
        }

        plane_t p;

        GetFacePlane( ( face_t )i, p );

        float fx = p.normal.x * ray.x;
        float fy = p.normal.y * ray.y;
        float fz = p.normal.z * ray.z;

        float thedot = fx + fy + fz;

        // if true then face faces away from ray, so move on
        if ( thedot >= 0.0f )
        {
            continue;
        }

        float t = -( glm::dot( origin, p.normal ) - p.d ) / thedot;

        if ( isinf( t ) )
        {
            continue;
        }

        glm::vec3 r( origin + ray * t );

        // only one component can be nonzero, so we test
        // against our current face to ensure that we're not outside of the bounds
        // of the face

        // If the origin is in the range of the corresponding
        // face's axis, this implies that we're wasting our time: the origin
        // isn't actually inside of the bounds, and since we've ommitted
        // all normals which are within 90 degrees or less of the ray,
        // the only way this face can be hit is if we negate the ray, which we don't want.

        // front or back face
        if ( fz != 0.0f && !isinf( fz ) )
        {
            if ( !InXRange( r ) ) continue;
            if ( !InYRange( r ) ) continue;
        }
        // top or bottom face
        else if ( fy != 0.0f && !isinf( fy ) )
        {
            if ( !InZRange( r ) ) continue;
            if ( !InXRange( r ) ) continue;
        }
        // left or right face
        else if ( fx != 0.0f && !isinf( fx ) )
        {
            if ( !InZRange( r ) ) continue;
            if ( !InYRange( r ) ) continue;
        }
        else
        {
            continue;
        }

        //faces[ fcount ] = std::move( p );
        intersections[ fcount++ ] = t;

        //fcount++;
    }

    // find closest intersection
    float t0 = FLT_MAX;

    for ( int32_t i = 0; i < fcount; ++i )
    {
        if ( intersections[ i ] < t0 )
        {
            t0 = intersections[ i ];
        }
    }

    return t0;
}

#define A_CalcEdge( e ) ( glm::normalize( e ) )
void aabb_t::GetFacePlane( face_t face, plane_t& plane ) const
{
/*

Future reference for OBBs...

    glm::vec3 e0, e1, p;

    switch ( face )
    {
    case AABB::FACE_TOP:
        p  = maxPoint;
        e0 = A_CalcEdge( p - Corner( CORNER_NEAR_UP_RIGHT ) );
        e1 = A_CalcEdge(  Corner( CORNER_FAR_UP_LEFT ) - p );
        break;
    case AABB::FACE_RIGHT:
        p  = maxPoint;
        e0 = p - Corner( CORNER_FAR_DOWN_RIGHT );
        e1 = Corner( CORNER_NEAR_UP_RIGHT ) - p;
        break;
    case AABB::FACE_FRONT:
        p  = Corner( CORNER_NEAR_UP_RIGHT );
        e0 = A_CalcEdge( p - Corner( CORNER_NEAR_DOWN_RIGHT ) );
        e1 = A_CalcEdge( Corner( CORNER_NEAR_UP_LEFT ) - p );
        break;
    case AABB::FACE_LEFT:
        p  = Corner( CORNER_NEAR_UP_LEFT );
        e0 = A_CalcEdge( p - Corner( CORNER_MIN ) );
        e1 = A_CalcEdge( Corner( CORNER_FAR_UP_LEFT ) - p );
        break;
    case AABB::FACE_BACK:
        p  = Corner( CORNER_FAR_UP_LEFT );
        e0 = A_CalcEdge( p - Corner( CORNER_FAR_DOWN_LEFT ) );
        e1 = A_CalcEdge( Corner( CORNER_MAX ) - p );
        break;
    case AABB::FACE_BOTTOM:
        p = Corner( CORNER_NEAR_DOWN_RIGHT );
        e0 = A_CalcEdge( p - Corner( CORNER_FAR_DOWN_RIGHT ) );
        e1 = A_CalcEdge( Corner( CORNER_MIN ) - p );
        break;
    }
*/
    glm::vec3 p;

    switch ( face )
    {
        case aabb_t::FACE_TOP:
            p  = maxPoint;
            plane.normal = glm::vec3( 0.0f, 1.0f, 0.0f );
            break;
        case aabb_t::FACE_RIGHT:
            p  = maxPoint;
            plane.normal = glm::vec3( 1.0f, 0.0f, 0.0f );
            break;
        case aabb_t::FACE_FRONT:
            p  = Corner( CORNER_NEAR_UP_RIGHT );
            plane.normal = glm::vec3( 0.0f, 0.0f, 1.0f );
            break;
        case aabb_t::FACE_LEFT:
            p  = Corner( CORNER_NEAR_UP_LEFT );
            plane.normal = glm::vec3( -1.0f, 0.0f, 0.0f );
            break;
        case aabb_t::FACE_BACK:
            p  = Corner( CORNER_FAR_UP_LEFT );
            plane.normal = glm::vec3( 0.0f, 0.0f, -1.0f );
            break;
        case aabb_t::FACE_BOTTOM:
            p = Corner( CORNER_NEAR_DOWN_RIGHT );
            plane.normal = glm::vec3( 0.0f, -1.0f, 0.0f );
            break;
    }

    plane.d = glm::dot( p, plane.normal );
}
#undef A_CalcEdge

static const float AABB_SIZE_FACTOR = 1.5f;

void aabb_t::FromTransform( aabb_t &box, const glm::mat4 &transform )
{
    // Compute our AABB using -

    // iter->world's scaling:
    float sx, sy, sz;

    sx = transform[ 0 ][ 0 ] * AABB_SIZE_FACTOR; // ensure our AABB is just *slightly* larger than our object.
    sy = transform[ 1 ][ 1 ] * AABB_SIZE_FACTOR;
    sz = transform[ 2 ][ 2 ] * AABB_SIZE_FACTOR;

    // and iter->world's translation:
    float tx, ty, tz;

    tx = transform[ 3 ][ 0 ];
    ty = transform[ 3 ][ 1 ];
    tz = transform[ 3 ][ 2 ];

    box.maxPoint.x = tx + sx;
    box.maxPoint.y = ty + sy;
    box.maxPoint.z = tz + sz;

    box.minPoint.x = tx - sx;
    box.minPoint.y = ty - sy;
    box.minPoint.z = tz - sz;
}

void aabb_t::FromPoints( aabb_t& box, const glm::vec3 v[], int32_t n )
{
    for ( int32_t i = 0; i < n; ++i )
    {
        box.Add( v[ i ] );
    }
}

//-------------------------------------------------------------------------------------------------------
// Frustum
//-------------------------------------------------------------------------------------------------------
#define _DEBUG_FRUSTUM

frustum_t::frustum_t( void )
    :	acceptCount( 0 ),
        rejectCount( 0 ),
        mvp( 1.0f )
{
    memset( frustPlanes, 0, sizeof( plane_t ) * FRUST_NUM_PLANES );
}

frustum_t::~frustum_t( void )
{
}

glm::vec4 frustum_t::CalcPlaneFromOrigin( const glm::vec4& position, const glm::vec4& origin )
{
    glm::vec4 plane( 0.0f );
    plane.x = position.x;
    plane.y = position.y;
    plane.z = position.z;
    plane	= glm::normalize( plane );
    plane.w = glm::dot( glm::vec3( origin ), glm::vec3( position ) );

    return plane;
}

#define F_CalcDist( plane ) ( ( plane ).d / glm::length( ( plane ).normal ) )
#define F_CalcNormal( a, b ) ( glm::cross( a, b ) )
void frustum_t::Update( const view::params_t& view )
{
    float tanHalfFovy = glm::tan( view.fovy * 0.5f );

    // We compute the reference angle since we want to base the sin/cosine on the angle from the x-axis;
    // without we have an angle from the z.
    float fov = glm::atan( view.aspect * 0.75f * tanHalfFovy );

    glm::vec3 u( view.inverseOrient[ 0 ] * glm::cos( fov ) );
    glm::vec3 v( view.inverseOrient[ 2 ] * -glm::sin( fov ) );
    glm::vec3 w( view.inverseOrient[ 1 ] );

    glm::vec3 planeLine( u + v );

    frustPlanes[ FRUST_RIGHT ].normal = F_CalcNormal( planeLine, -w );
    frustPlanes[ FRUST_RIGHT ].d = glm::dot( view.origin + planeLine, frustPlanes[ FRUST_RIGHT ].normal );

    planeLine = -u + v;

    frustPlanes[ FRUST_LEFT  ].normal = F_CalcNormal( planeLine, w );
    frustPlanes[ FRUST_LEFT  ].d = glm::dot( view.origin + planeLine, frustPlanes[ FRUST_LEFT ].normal );

    // Z is the initial axis for the horizontal planes
    fov = glm::atan( tanHalfFovy );

    u = -glm::vec3( view.inverseOrient[ 2 ] * glm::cos( fov ) );
    v = glm::vec3( view.inverseOrient[ 1 ] * -glm::sin( fov ) );
    w = glm::vec3( view.inverseOrient[ 0 ] );

    planeLine = -u + v;
    frustPlanes[ FRUST_TOP ].normal = F_CalcNormal( w, planeLine );
    frustPlanes[ FRUST_TOP ].d = glm::dot( view.origin + planeLine, frustPlanes[ FRUST_TOP ].normal );

    planeLine = u + v;
    frustPlanes[ FRUST_BOTTOM ].normal = F_CalcNormal( w, planeLine );
    frustPlanes[ FRUST_BOTTOM ].d = glm::dot( view.origin + planeLine, frustPlanes[ FRUST_BOTTOM ].normal );
}
#undef F_CalcNormal

// Adding plane[ 3 ] ( which is the distance from the plane to the origin ) offsets the plane so we can ensure that the point is in front of the plane normal

#ifdef _DEBUG_FRUSTUM
    static float F_PlaneSide( const glm::vec3& point, const plane_t& plane )
    {
        float x = glm::dot( point, plane.normal ) - plane.d;

        return x;
    }
#else
#	define F_PlaneSide( point, plane ) ( glm::dot( ( point ), ( plane ).normal ) - ( plane ).d )
)
#endif

bool frustum_t::IntersectsBox( const aabb_t& box ) const
{
#define C(v) ( glm::vec3( ( v ) ) )

    std::array< glm::vec3, 8 > clipBounds =
    {{
        C( box.Corner4( 0 ) ),
        C( box.Corner4( 1 ) ),
        C( box.Corner4( 2 ) ),
        C( box.Corner4( 3 ) ),
        C( box.Corner4( 4 ) ),
        C( box.Corner4( 5 ) ),
        C( box.Corner4( 6 ) ),
        C( box.Corner4( 7 ) )
    }};
#undef C

    // Test each corner against every plane normal
    for ( int i = 0; i < 4; ++i )
    {
        if ( F_PlaneSide( clipBounds[ 0 ], frustPlanes[ i ] ) >= 0 ) continue;
        if ( F_PlaneSide( clipBounds[ 1 ], frustPlanes[ i ] ) >= 0 ) continue;
        if ( F_PlaneSide( clipBounds[ 2 ], frustPlanes[ i ] ) >= 0 ) continue;
        if ( F_PlaneSide( clipBounds[ 3 ], frustPlanes[ i ] ) >= 0 ) continue;
        if ( F_PlaneSide( clipBounds[ 4 ], frustPlanes[ i ] ) >= 0 ) continue;
        if ( F_PlaneSide( clipBounds[ 5 ], frustPlanes[ i ] ) >= 0 ) continue;
        if ( F_PlaneSide( clipBounds[ 6 ], frustPlanes[ i ] ) >= 0 ) continue;
        if ( F_PlaneSide( clipBounds[ 7 ], frustPlanes[ i ] ) >= 0 ) continue;

        rejectCount++;
        return false;
    }

    acceptCount++;
    return true;
}

#ifdef F_PlaneSide
#	undef F_PlaneSide
#endif


} // namespace geom
