#ifndef TWOHALFD_ANIMATION_TYPES_H
#define TWOHALFD_ANIMATION_TYPES_H

#include <vector>

namespace TwoHalfD {

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
} // namespace TwoHalfD

#endif