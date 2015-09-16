#pragma once

#include <glm/glm.hpp>
#include <stdint.h>

#include "geom_point_project_pair.h"

namespace std {

//----------------------------------------
// glm::vec3 specializations
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

//----------------------------------------
// point_project_pair specializations
//----------------------------------------

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
