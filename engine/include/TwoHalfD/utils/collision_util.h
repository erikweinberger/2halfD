#ifndef COLLISION_UTIL_H
#define COLLISION_UTIL_H
#include "../engine_types.h"

namespace TwoHalfD {

// Calculates how much the circleSegment penetrates over the segment and gives back vector
// for circle to move along to no longer overlap
XYVectorf resolveCircleVsSegment(XYVectorf pos, float radius, const Segment &seg);

// Calculates circle overlap and give vector to move circle one so circles no longer overlap
XYVectorf resolveCircleVsCircle(XYVectorf pos, float r1, XYVectorf otherPos, float r2);
} // namespace TwoHalfD

#endif