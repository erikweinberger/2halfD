#ifndef PHYSICS_UTIL_H
#define PHYSICS_UTIL_H

#include "TwoHalfD/types/entity_types.h"
#include "TwoHalfD/types/math_types.h"

#include <limits>
#include <utility>
#include <vector>

namespace TwoHalfD {

std::pair<const Wall *, float> findNearestWall(const XYVectorf &cord, const XYVectorf &rayDir, const std::vector<Wall> &walls);

std::pair<const Wall *, float> findNearestWall(const Position &ray, const std::vector<Wall> &walls);

} // namespace TwoHalfD

#endif
