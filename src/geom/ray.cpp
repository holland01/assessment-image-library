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

void ray::draw( imm_draw& drawer, const glm::vec4& color ) const
{
    drawer.begin( GL_LINES );
    drawer.vertex( mOrigin, color );
    drawer.vertex( mDir * mT, color );
    drawer.vertex( mOrigin, color );
    drawer.vertex( mDir * -mT, color );
    drawer.end();
}
