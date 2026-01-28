#ifndef MATHUTIL_H
#define MATHUTIL_H

#include "TwoHalfD/engine_types.h"

#include <array>
#include <optional>
#include <vector>

using point2d = std::array<float, 2>;

static const float PI_f = std::numbers::pi_v<float>;

inline float degreeToRad(float degree) {
    return degree * PI_f / 180;
}

inline float radToDegree(float rad) {
    return 180.f / PI_f * rad;
}

inline float lengthOfVector(const TwoHalfD::XYVectorf &v) {
    return std::sqrt(v.x * v.x + v.y * v.y);
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

inline float crossProduct2d(const TwoHalfD::XYVectorf &v, const TwoHalfD::XYVectorf &u) {
    return v.x * u.y - u.x * v.y;
}

inline float dotProduct(const TwoHalfD::XYVectorf &a, const TwoHalfD::XYVectorf &b) {
    return a.x * b.x + a.y * b.y;
}

inline bool isInfront(const TwoHalfD::XYVectorf &v, const TwoHalfD::XYVectorf &u) {
    return v.x * u.y < u.x * v.y;
}

inline bool isBehind(const TwoHalfD::XYVectorf &v, const TwoHalfD::XYVectorf &u) {
    return v.x * u.y > u.x * v.y;
}

std::vector<point2d> findCircleLineSegmentIntercept(const float cx, const float cy, const float r, const point2d &wallS, const point2d &wallE);

#endif