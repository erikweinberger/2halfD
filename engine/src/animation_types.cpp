#include "TwoHalfD/types/animation_types.h"
#include <algorithm>

int TwoHalfD::OverlayStack::add(int templateId, float x, float y, float scale, int zOrder, bool loop) {
    int id = nextId++;

    if (count < MAX_OVERLAYS) {
        size_t idx = count++;
        overlays[idx] = OverlayInstance{
            AnimationState{templateId, 0, 0.f, loop},
            x, y, scale, zOrder, id, true
        };
        sortByZOrder();
        return id;
    }

    // Full — replace the first element (lowest zOrder, farthest back)
    overlays[0] = OverlayInstance{
        AnimationState{templateId, 0, 0.f, loop},
        x, y, scale, zOrder, id, true
    };
    sortByZOrder();
    return id;
}

void TwoHalfD::OverlayStack::remove(int overlayId) {
    for (size_t i = 0; i < count; ++i) {
        if (overlays[i].overlayId == overlayId) {
            overlays[i] = overlays[count - 1];
            overlays[count - 1].active = false;
            --count;
            sortByZOrder();
            return;
        }
    }
}

void TwoHalfD::OverlayStack::clear() {
    for (size_t i = 0; i < count; ++i) {
        overlays[i].active = false;
    }
    count = 0;
}

void TwoHalfD::OverlayStack::sortByZOrder() {
    std::sort(overlays.begin(), overlays.begin() + count,
              [](const OverlayInstance &a, const OverlayInstance &b) {
                  return a.zOrder < b.zOrder;
              });
}
