#ifndef BSP_TYPES_H
#define BSP_TYPES_H

#include "TwoHalfD/types/entity_types.h"

#include <memory>
#include <unordered_set>
#include <vector>

namespace TwoHalfD {

struct Segment {
    XYVectorf v1;
    XYVectorf v2;

    const Wall *wall;
    const FloorSection *floorSection;
    float wallRatioStart{0.f};
    float wallRatioEnd{1.f};

    bool isWall() const {
        return wall != nullptr;
    }

    bool isFloorBoundary() const {
        return floorSection != nullptr;
    }
};

struct BSPNode {
    std::unique_ptr<BSPNode> front;
    std::unique_ptr<BSPNode> back;

    std::unordered_set<int> spriteIds;
    Polygon bounds;
    std::unique_ptr<FloorSection> floorSection = nullptr;

    XYVectorf splitterP0;
    XYVectorf splitterP1;
    XYVectorf splitterVec;

    int segmentID = -1;
};

struct DrawCommand {
    enum class Type {
        Segment,
        Sprite,
        FloorSection
    } type;

    union {
        int id;                        // For Segment or Sprite
        FloorSection *floorSectionPtr; // For FloorSection
    };

    static DrawCommand makeSegment(int segId) {
        DrawCommand cmd;
        cmd.type = Type::Segment;
        cmd.id = segId;
        return cmd;
    }

    static DrawCommand makeSprite(int spriteId) {
        DrawCommand cmd;
        cmd.type = Type::Sprite;
        cmd.id = spriteId;
        return cmd;
    }

    static DrawCommand makeFloorSection(FloorSection *ptr) {
        DrawCommand cmd;
        cmd.type = Type::FloorSection;
        cmd.floorSectionPtr = ptr;
        return cmd;
    }
};

} // namespace TwoHalfD

#endif
