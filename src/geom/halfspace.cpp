#include "halfspace.h"
#include "../renderer.h"
#include "obb.h"
#include "geom_util.h"

namespace {
    void shape_deleter( btBox2dShape* s )
    {
        delete s;
    }
}

halfspace::halfspace( void )
	: halfspace( glm::mat3( 1.0f ), glm::vec3( 0.0f ), 0.0f )
{
}

halfspace::halfspace( const glm::mat3& axes, const glm::vec3& origin, float distance_ )
    :   bounds_primitive( BOUNDS_PRIM_HALFSPACE ),
        mAxes( glm::ext::to_bullet( axes ), glm::ext::to_bullet( origin ) ),
        mDistance( distance_ ),
        mBox( new btBox2dShape( mAxes.getBasis()[ 0 ] + mAxes.getBasis()[ 1 ] + mAxes.getBasis()[ 2 ] ), shape_deleter )
{
}

halfspace::halfspace( const obb& bounds, const glm::vec3& normal )
	: halfspace( glm::mat3( 1.0f ), glm::vec3( 0.0f ), 0.0f )
{
	using corner_t = obb::corner_type;

	glm::vec3 upAxis( bounds.axes()[ 1 ] );
	glm::vec3 boundsOrigin( bounds.origin() );
	glm::vec3 boundsSize( bounds.extents() * 2.0f );

    btMatrix3x3& ax = mAxes.getBasis();

	// normalize the cross on mAxes[ 0 ] so that we don't scale more than is necessary
    ax[ 0 ] = glm::ext::to_bullet( glm::normalize( glm::cross( normal, upAxis ) ) ) * boundsSize[ 0 ];
    ax[ 1 ] = glm::ext::to_bullet( upAxis * boundsSize[ 1 ] );
    ax[ 2 ] = glm::ext::to_bullet( normal );

	// normal is assumed to be a cardinal (positive or negative) axis, so multiplying
	// by boundsSizes will distribute the intended length along the appropriate component, where as all others will be zero
	// as before. We then transform the scaled normal, since scaling before transformation will keep the length preserved (as opposed to after).
	glm::vec3 faceCenter( bounds.axes() * ( glm::normalize( normal ) * boundsSize * 0.5f ) );
	faceCenter += boundsOrigin; // offset by boundsOrigin since we need its worldSpace position

	// TODO: take into account ALL points
	std::array< corner_t, 4 > lowerPoints =
	{{
		obb::CORNER_MIN,
		obb::CORNER_NEAR_DOWN_RIGHT,
		obb::CORNER_FAR_DOWN_RIGHT,
		obb::CORNER_FAR_DOWN_LEFT
	}};

    glm::vec3 btX( glm::ext::from_bullet( ax[ 0 ] ) );
    glm::vec3 btY( glm::ext::from_bullet( ax[ 1 ] ) );

	for ( corner_t face: lowerPoints )
	{
		glm::vec3 point( bounds.world_corner( face ) );
		glm::vec3 pointToCenter( faceCenter - point );

		// Not in same plane; move on
        if ( triple_product( pointToCenter, btX, btY ) != 0.0f )
		{
			continue;
		}

		// Half space axes will be outside of the bounds; move on
        if ( !bounds.range( point + btX, false ) || !bounds.range( point + btY, false ) )
		{
			continue;
		}

        mAxes.setOrigin( glm::ext::to_bullet( point ) );

		break;
	}

    ax[ 2 ].normalize();

    mDistance = glm::dot( glm::ext::from_bullet( ax[ 2 ] ),
                          glm::ext::from_bullet( mAxes.getOrigin() ) );
}

halfspace::halfspace( const halfspace& hs )
    : bounds_primitive( BOUNDS_PRIM_HALFSPACE ),
      mAxes( hs.mAxes ),
      mDistance( hs.mDistance ),
      mBox( new btBox2dShape( hs.mBox->getHalfExtentsWithoutMargin() ), shape_deleter )
{
}

halfspace& halfspace::operator= ( const halfspace& hs )
{
    if ( this != &hs )
    {
        mAxes = hs.mAxes;
        mDistance = hs.mDistance;
        mBox.reset( new btBox2dShape( hs.mBox->getHalfExtentsWithoutMargin() ) );
    }

    return *this;
}

void halfspace::draw( imm_draw& drawer ) const
{
	drawer.begin( GL_LINES );

	drawer.vertex( origin() );
	drawer.vertex( origin() + axes()[ 0 ] );

	drawer.vertex( origin() );
	drawer.vertex( origin() + axes()[ 1 ] );

	drawer.vertex( origin() );
	drawer.vertex( origin() + axes()[ 2 ] * 2.0f );

	drawer.end();

	drawer.begin( GL_POINTS );
	drawer.vertex( origin() );
	drawer.end();
}
