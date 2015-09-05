#pragma once

#include <glm/glm.hpp>
#include <stdint.h>

namespace std {

//----------------------------------------
// vec3 specializations
//----------------------------------------

    template <> struct hash< glm::vec3 >
    {
        size_t operator()( const glm::vec3& x ) const
        {
            size_t a = size_t( glm::length( x ) );

            return std::hash< size_t >()( a );
        }
    };

    template<> struct equal_to< glm::vec3 >
    {
        bool operator()( const glm::vec3& lhs, const glm::vec3& rhs ) const
        {
            return lhs == rhs;
        }
    };

    template<> struct less< glm::vec3 >
    {
        bool operator()( const glm::vec3& lhs, const glm::vec3& rhs ) const
        {
            return glm::all( glm::lessThan( lhs, rhs ) );
        }
    };
}
