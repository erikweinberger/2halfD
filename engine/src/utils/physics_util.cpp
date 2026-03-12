#include "TwoHalfD/utils/physics_util.h"

#include <cmath>
#include <limits>

std::pair<const TwoHalfD::Wall *, float> TwoHalfD::findNearestWall(const XYVectorf &cord, const XYVectorf &rayDir,
                                                                    const std::vector<Wall> &walls) {
    const TwoHalfD::Wall *nearestWall = nullptr;
    float shortestDist = std::numeric_limits<float>::max();

    for (const auto &wall : walls) {
        float x1 = cord.x, y1 = cord.y;
        float x2 = x1 + 1000.0f * rayDir.x, y2 = y1 + 1000.0f * rayDir.y;

        float x3 = wall.start.x, y3 = wall.start.y;
        float x4 = wall.end.x, y4 = wall.end.y;

        float denom = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
        if (std::abs(denom) < 0.00001f) continue;

        float numeratorT = (x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4);
        float numeratorU = -((x1 - x2) * (y1 - y3) - (y1 - y2) * (x1 - x3));

        float t = numeratorT / denom;
        float u = numeratorU / denom;

        if (u < 0 || u > 1 || t < 0) continue;

        if (t < shortestDist) {
            shortestDist = t;
            nearestWall = &wall;
        }
    }
    return {nearestWall, nearestWall == nullptr ? std::numeric_limits<float>::max() : shortestDist * 1000};
}

std::pair<const TwoHalfD::Wall *, float> TwoHalfD::findNearestWall(const Position &ray, const std::vector<Wall> &walls) {
    XYVectorf dirVector{std::cos(ray.direction), std::sin(ray.direction)};
    return findNearestWall(ray.pos, dirVector, walls);
}
