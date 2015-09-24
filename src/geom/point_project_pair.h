#pragma once

#include "plane.h"

//-------------------------------------------------------------------------------------------------------
// point_project_pair
//-------------------------------------------------------------------------------------------------------

struct point_project_pair
{
    const plane& mPlaneRef;
    const glm::vec3& mWorldPointRef;
    const glm::vec3 mProjected;
    const float mDistToPlane;

    point_project_pair( const plane& thePlane,
                        const glm::vec3& point )
        : mPlaneRef( thePlane ),
          mWorldPointRef( point ),
          mProjected( std::move( plane_project( point, thePlane ) ) ),
          mDistToPlane( glm::dot( thePlane.mNormal, point ) - thePlane.mDistance )
    {
    }

	// Compares this distance to the plane to x's distance to the plane
    bool closer_than( const point_project_pair& x ) const;
};

//
// Overloads for STL containers
//

// We only compare projected and plane values so that if two
// point_project_pairs hold the same plane and projected values, but have
// differing world points, they'll still be considered the same within an STL set
INLINE bool operator == ( const point_project_pair& a, const point_project_pair& b )
{
    return a.mPlaneRef.similar_to( b.mPlaneRef ) && a.mProjected == b.mProjected;
}

// Implemented for STL container usage
INLINE bool operator < ( const point_project_pair& l, const point_project_pair& r )
{
    return l.closer_than( r );
}

INLINE bool point_project_pair::closer_than( const point_project_pair& x ) const
{
    return *this == x && glm::abs( mDistToPlane ) < glm::abs( x.mDistToPlane );
}

//
// STL specializations
//

namespace std {

template<> struct hash< point_project_pair >
{
    size_t operator()( const point_project_pair& x ) const
    {
        return hash< glm::vec3 >()( x.mProjected );
    }
};

template<> struct equal_to< point_project_pair >
{
    bool operator()( const point_project_pair& lhs, const point_project_pair& rhs ) const
    {
        return lhs == rhs;
    }
};

template<> struct less< point_project_pair >
{
    bool operator()( const point_project_pair& lhs, const point_project_pair& rhs ) const
    {
        return lhs < rhs;
    }
};

}
