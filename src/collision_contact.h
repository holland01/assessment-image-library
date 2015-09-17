#pragma once

#include "def.h"
#include <glm/glm.hpp>
#include <unordered_set>

//----------------------------------------
// contact
//----------------------------------------

struct contact
{
    using list_t = std::unordered_set< contact >;

    const glm::vec3 mPoint;
    const glm::vec3 mNormal;
    const float mInterpenDepth;

    contact( const glm::vec3 point = glm::vec3( 0.0f ),
             const glm::vec3 normal = glm::vec3( 0.0f ),
             const float interpenDepth = 0.0f )
      : mPoint( std::move( point ) ),
        mNormal( std::move( normal ) ),
        mInterpenDepth( interpenDepth )
    {
    }
};

//
// Operator overloads for STL set usage
//

INLINE bool operator == ( const contact& a, const contact& b )
{
    return a.mPoint == b.mPoint && a.mNormal == b.mNormal && a.mInterpenDepth == b.mInterpenDepth;
}

// No particular reason for comparing via normal length; it just seems like
// the only useful heuristic when using in an ordered container
INLINE bool operator < ( const contact& a, const contact& b )
{
    return glm::length( a.mNormal ) < glm::length( b.mNormal );
}

//
// STL specializations for set usage
//

namespace std {

template<> struct hash< contact >
{
    size_t operator()( const contact& x ) const
    {
        return ( size_t ) glm::abs( hash< glm::vec3 >()( x.mPoint ) + hash< glm::vec3 >()( x.mNormal ) );
    }
};

template<> struct equal_to< contact >
{
    bool operator()( const contact& lhs, const contact& rhs ) const
    {
        return lhs == rhs;
    }
};

template<> struct less< contact >
{
    bool operator()( const contact& lhs, const contact& rhs ) const
    {
        return lhs < rhs;
    }
};

}
