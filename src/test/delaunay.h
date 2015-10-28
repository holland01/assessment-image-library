#pragma once

#include "../def.h"
#include APPLICATION_BASE_HEADER

// (NOTE)
// To find the circumcircle:
// 1.
// For each triangle edge, create a bisector which extends into the triangle from the center of the edge.
// Each bisector should be perpendicular to its corresponding edge. Let the length of every bisector be
// the sum of the lengths of all edges squared. The intersection point between the three bisectors
// is a reference point: the length of any direction from that point to the one of the triangle vertices
// defines the radius of the triangle's circle.
// 2.
// Make sure each triangle vertex touches the outer perimeter/circumference of the circle.
//
// As long as the radius is correct, and the condition in 2. is met, you should be good to go.

struct delaunay_test;
using dt_app_t = application< delaunay_test >;

struct dtri
{
    std::array< glm::vec3, 3 > mVerts;
    glm::vec3 mCircumCenter;
    float mCircumRadius;

    dtri( const std::array< glm::vec3, 3 >& verts, const glm::vec3& circumCircleCenter, float circumCircleRadius )
        : mVerts( verts ),
          mCircumCenter( circumCircleCenter ),
          mCircumRadius( circumCircleRadius )
    {
    }
};

struct dnode
{
    glm::vec3 mPoint;
    glm::vec4 mColor;

    std::unique_ptr< dtri > mTriangle;

    dnode( const glm::vec3& point, const glm::vec4& color )
        : mPoint( point ),
          mColor( color ),
          mTriangle( nullptr )
    {
    }
};

FORCEINLINE bool operator == ( const dnode& a, const dnode& b )
{
    return a.mPoint == b.mPoint &&
        a.mColor == b.mColor &&
        a.mTriangle == b.mTriangle;
}

class delaunay_test : public dt_app_t
{
private:
    std::vector< dnode > mNodes;

public:
    delaunay_test( uint32_t width, uint32_t height );

    void draw( void ) override;

    void frame( void ) override;

    dtri* gen_triangle( const glm::vec3& position );

    void draw_nodes( imm_draw& draw, const shader_program& prog );
};

