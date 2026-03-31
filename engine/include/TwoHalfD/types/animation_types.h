#ifndef TWOHALFD_ANIMATION_TYPES_H
#define TWOHALFD_ANIMATION_TYPES_H

#include <array>
#include <cstddef>
#include <vector>

namespace TwoHalfD {

inline constexpr size_t MAX_OVERLAYS = 10;

struct AnimationFrame {
    int textureId;
    float duration; // Duration of the frame in seconds
};

struct AnimationTemplate {
    int id;
    std::vector<AnimationFrame> frames;
};

struct AnimationState {
    int templateId; // which AnimationTemplate is playing
    int frameIndex = 0;
    float elapsedTime = 0.f; // time into current frame
    bool loop = false;
};

struct OverlayInstance {
    AnimationState animState;
    float x = 0.5f;     // 0-1 relative to sprite width (0=left, 1=right)
    float y = 0.5f;     // 0-1 relative to sprite height (0=top, 1=bottom)
    float scale = 1.f;
    int zOrder = 0;      // lower = drawn first (behind)
    int overlayId = -1;  // stable ID for external reference
    bool active = false; // whether this slot is in use
};

struct OverlayStack {
    std::array<OverlayInstance, MAX_OVERLAYS> overlays{};
    size_t count = 0;
    int nextId = 0;

    // Returns stable overlay ID (or -1 on failure)
    int add(int templateId, float x, float y, float scale, int zOrder, bool loop);
    void remove(int overlayId);
    void clear();
    void sortByZOrder();
};

} // namespace TwoHalfD

#endif