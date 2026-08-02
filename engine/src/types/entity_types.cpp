
#include "TwoHalfD/types/entity_types.h"
#include "TwoHalfD/types/bsp_types.h"

namespace TwoHalfD {

float PerimeterPoint::height() const {
    return bspRegion ? bspRegion->height() : 0.f;
}

} // namespace TwoHalfD