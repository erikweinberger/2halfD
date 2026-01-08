#ifndef MATHUTIL_H
#define MATHUTIL_H

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

std::vector<point2d> findCircleLineSegmentIntercept(const float cx, const float cy, const float r, const point2d &wallS, const point2d &wallE);

#endif