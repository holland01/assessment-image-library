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

class delaunay_test : public dt_app_t
{
public:
    delaunay_test( uint32_t width, uint32_t height );

    void draw( void ) override;

    void frame( void ) override;
};

