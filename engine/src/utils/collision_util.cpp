#include <TwoHalfD/utils/collision_util.h>

namespace TwoHalfD {

XYVectorf resolveCircleVsSegment(XYVectorf pos, float radius, const Segment &seg) {
    auto segVec = seg.v2 - seg.v1;
    float t = dot(pos - seg.v1, segVec) / segVec.lengthSquared();
    t = std::clamp(t, 0.f, 1.f);
    auto closestPoint = seg.v1 + t * segVec;
    auto pushVec = pos - closestPoint;
    float dist = pushVec.length();
    if (dist < radius && dist > 0.f) {
        float penetrationDepth = (radius - dist + 1.f);
        return (pushVec / dist) * penetrationDepth;
    }
    return {0.f, 0.f};
}

XYVectorf resolveCircleVsCircle(XYVectorf pos, float r1, XYVectorf otherPos, float r2) {
    auto c1Toc2Vector = pos - otherPos;
    float c1Toc2Dist = c1Toc2Vector.length();

    if (c1Toc2Dist <= r1 + r2 && c1Toc2Dist > 0.f) {
        return c1Toc2Vector / c1Toc2Dist * (r1 + r2 - c1Toc2Dist + 1.f);
    }
    return {0.f, 0.f};
}
} // namespace TwoHalfD