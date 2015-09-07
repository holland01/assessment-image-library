#pragma once

#include "def.h"
#include "geom.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <array>

#ifdef EMSCRIPTEN
#	define OP_DEFAULT_MOVE_STEP 0.5f
#else
#	define OP_DEFAULT_MOVE_STEP 1.0f
#endif

struct view_data
{
    glm::vec3   mForward;
    glm::vec3   mUp;
    glm::vec3   mRight;

    glm::vec3   mOrigin;

    glm::vec3   mCurrRot, mLastRot;

    glm::vec3   mLastMouse;

    float       mFovy, mAspect, mZNear, mZFar;
    float		mWidth, mHeight;
    float		mMoveStep;

    glm::mat4   mTransform;

    glm::mat4   mOrientation;
    glm::mat4   mInverseOrient;
    glm::mat4   mLastOrientation;

    glm::mat4   mClipTransform;

    view_data( void );
};


//-------------------------------------------------------------------------------------------------------
// Frustum
//-------------------------------------------------------------------------------------------------------

#define FRUST_NUM_PLANES 6

enum
{
	FRUST_NONE      = 6,
	FRUST_TOP       = 0,
	FRUST_BOTTOM    = 1,
	FRUST_RIGHT     = 2,
	FRUST_LEFT      = 3,
	FRUST_NEAR      = 4,
	FRUST_FAR       = 5
};

struct view_frustum
{
    plane    mFrustPlanes[ FRUST_NUM_PLANES ];

    mutable uint32_t mAcceptCount;

    mutable uint32_t mRejectCount;

    glm::mat4 mMvp;

    glm::vec4 get_plane_from_origin( const glm::vec4& position, const glm::vec4& origin );

    view_frustum( void );

    ~view_frustum( void );

    void    update( const view_data& params );

    void	print_metrics( void ) const;

    void	reset_metrics( void ) const { mRejectCount = 0; mAcceptCount = 0; }

    bool    intersects( const obb& box ) const;
};

INLINE void view_frustum::print_metrics( void ) const
{
    printf( "Reject Count: %iu; Accept Count: %iu\r", mRejectCount, mAcceptCount );
}
