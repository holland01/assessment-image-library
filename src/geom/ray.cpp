#include "ray.h"
#include "renderer.h"

ray::ray( const glm::vec3& position, const glm::vec3& dir, float t_ )
    : mOrigin( position ),
      mDir( dir ),
      mT( t_ )
{}

ray::ray( const ray& r )
    : mOrigin( r.mOrigin ),
      mDir( r.mDir ),
      mT( r.mT )
{
}

ray& ray::operator=( const ray& r )
{
    if ( this != &r )
    {
        mOrigin = r.mOrigin;
        mDir = r.mDir;
        mT = r.mT;
    }

    return *this;
}

bool ray::intersects( const ray& r, float& thisT, float& candidateT ) const
{
    // Zero these out to prevent garbage data issues...
    thisT = 0.0f;
    candidateT = 0.0f;

    // Important to remember that:
    // |cross( a, b )| = |a||b|sin(theta), where theta is the angle between the two vectors
    const float DISTANCE_THRESH = 0.03125f;

    glm::vec3 normal( glm::cross( mDir, r.mDir ) );

    float dist = glm::distance( mOrigin, r.mOrigin );

    // Here we check for parallelity and confirm that we're not intersecting
    if ( normal == glm::zero< glm::vec3 >() && dist > DISTANCE_THRESH  )
        return false;

    // Dir acts as hypotenuse
    glm::vec3 dirToThis( mOrigin - r.mOrigin );

    // Length of this will produce the distance from this origin to
    // somewhere in r's line of path (due to sine equivalence)
    glm::vec3 rNormal( glm::cross( r.mDir, dirToThis ) );

    // Likely normalized, however there may be slight deviations
    float normLen = glm::length( normal );
    float invNormLenSqr = 1.0f / ( normLen * normLen );

    // dot( rNormal, normal ) = |normal| |rNormal| cos(theta)
    // multiplying by invNormLenSqr produces |rNormal|/|normal| * cos(theta),
    // which should normalize the ray's dir if it isn't already normalized
    thisT = glm::dot( rNormal, normal ) * invNormLenSqr;

    // Same idea: length of thisNormal will produce distance from r's origin
    // to somewhere in this' line of path: the hypotenuse dir, dirToThis,
    // gives us the length we need to ensure this.
    glm::vec3 thisNormal( glm::cross( mDir, dirToThis ) );

    candidateT = glm::dot( thisNormal, normal ) * invNormLenSqr;

    glm::vec3 pr0( mOrigin + mDir * thisT );
    glm::vec3 pr1( r.mOrigin + r.mDir * candidateT );

    // Check for skew lines
    if ( glm::distance( pr0, pr1 ) > DISTANCE_THRESH )
        return false;

    return true;
}

void ray::draw( imm_draw& drawer, const glm::vec4& color ) const
{
    drawer.begin( GL_LINES );
    drawer.vertex( mOrigin, color );
    drawer.vertex( mDir * mT, color );
    drawer.vertex( mOrigin, color );
    drawer.vertex( mDir * -mT, color );
    drawer.end();
}
