#ifndef MATHUTIL_H
#define MATHUTIL_H

#include "TwoHalfD/engine_types.h"
#include "TwoHalfD/types/math_types.h"

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

inline bool isCollinear(const TwoHalfD::XYVectorf &v1, const TwoHalfD::XYVectorf &v2, const TwoHalfD::XYVectorf &u1, const TwoHalfD::XYVectorf &u2) {
    const auto d1 = v2 - v1;
    const auto d2 = u2 - u1;
    return std::abs(crossProduct2d(d1, d2)) < 1e-4f && std::abs(crossProduct2d(u1 - v1, d1)) < 1e-4f;
}

// Checks both collinear then uses this info to check if the segments overlap
inline bool segmentsOverlap(const TwoHalfD::XYVectorf &v1, const TwoHalfD::XYVectorf &v2, const TwoHalfD::XYVectorf &u1,
                            const TwoHalfD::XYVectorf &u2) {
    if (!isCollinear(v1, v2, u1, u2)) return false;
    const auto dir = (v2 - v1).normalized();
    float a0 = 0.f;
    float a1 = dotProduct(v2 - v1, dir);
    float b0 = dotProduct(u1 - v1, dir);
    float b1 = dotProduct(u2 - v1, dir);
    if (a0 > a1) std::swap(a0, a1);
    if (b0 > b1) std::swap(b0, b1);
    return b0 < a1 && a0 < b1;
}

std::vector<point2d> findCircleLineSegmentIntercept(const float cx, const float cy, const float r, const point2d &wallS, const point2d &wallE);

TwoHalfD::XYVectorf computeLineIntersection(const TwoHalfD::XYVectorf &p1, const TwoHalfD::XYVectorf &p2, const TwoHalfD::XYVectorf &p3,
                                            const TwoHalfD::XYVectorf &p4);

bool isCounterClockwise(const TwoHalfD::Polygon &polygon);

std::vector<TwoHalfD::XYVectorf> circleLineIntersect(const TwoHalfD::XYVectorf &center, float radius, const TwoHalfD::XYVectorf &lineP1,
                                                     const TwoHalfD::XYVectorf &lineP2);

#endif