#ifndef MATHUTIL_H
#define MATHUTIL_H

#include "TwoHalfD/engine_types.h"

#include <array>
#include <optional>
#include <vector>

using point2d = std::array<float, 2>;

static const float PI = 3.1415927;

inline float degreeToRad(float degree) {
    return degree * PI / 180;
}

inline float radToDegree(float rad) {
    return 180.f / PI * rad;
}

inline float distanceBetweenPoints(const TwoHalfD::XYVectorf &v, const TwoHalfD::XYVectorf &u) {
    return std::sqrt((v.x - u.x) * (v.x - u.x) + (v.y - u.y) * (v.y - u.y));
}

inline float distanceSquaredBetweenPoints(const TwoHalfD::XYVectorf &v, const TwoHalfD::XYVectorf &u) {
    return (v.x - u.x) * (v.x - u.x) + (v.y - u.y) * (v.y - u.y);
}

inline TwoHalfD::XYVectorf vectorBetweenPoints(const TwoHalfD::XYVectorf &start, const TwoHalfD::XYVectorf &end) {
    return {end.x - start.x, end.y - start.y};
}

std::vector<point2d> findCircleLineSegmentIntercept(const float cx, const float cy, const float r, const point2d &wallS, const point2d &wallE);

#endif