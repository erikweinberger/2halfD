#ifndef ENTITY_TYPES_H
#define ENTITY_TYPES_H

#include "TwoHalfD/types/animation_types.h"
#include "TwoHalfD/types/math_types.h"

#include <SFML/Graphics/Texture.hpp>
#include <cstddef>
#include <optional>
#include <string>
#include <variant>
#include <array>
#include <vector>

namespace TwoHalfD {

struct TextureSignature {
    sf::Texture texture;
    std::string filePath;
    int id;
};

struct Wall {
    XYVectorf start, end;
    int id;
    int textureId;
    float height;
    float wallHeightStart = 0.f;
    float scaleX = 1.f; // fraction of wall one texture copy fills horizontally
    float scaleY = 1.f; // fraction of wall one texture copy fills vertically
};

struct FloorSection {
    Polygon vertices;
    XYVectorf floorTextureStart;
    int id;
    int textureId;
    float height;
    bool isCCW;
};

struct FloorColourOverlay {
    Polygon vertices;
    int id;
    float height;
    uint8_t r, g, b, a;
};

struct WalkToUpdate {
    TwoHalfD::XYVectorf targetPos;
    TwoHalfD::Path path;
    size_t nextPathIndex = 0;
};

struct AttackUpdate {
    int targetEntityId;
};

struct IdleUpdate {};

using EntityUpdate = std::variant<WalkToUpdate, AttackUpdate, IdleUpdate>;

struct PerimeterPoint {
    TwoHalfD::XYVectorf offset;
    float floorHeight = 0.f;
};

struct SpriteEntity {
    int id;
    TwoHalfD::Position pos;
    float radius;
    int height;
    int textureId;
    float scaleX = 1.f; // fraction of sprite area one texture copy fills horizontally
    float scaleY = 1.f; // fraction of sprite area one texture copy fills vertically
    float heightStart = 0.f;
    std::array<PerimeterPoint, 4> perimeterPoints = {}; // initialized via initPerimeterPoints()

    void initPerimeterPoints() {
        perimeterPoints = {PerimeterPoint{{radius, 0}}, PerimeterPoint{{-radius, 0}}, PerimeterPoint{{0, radius}}, PerimeterPoint{{0, -radius}}};
    }

    float speed = 5.f;

    float floorHeight = 0.f; // updated by BSPManager on insert/move

    struct Velocity {
        float x = 0.f, y = 0.f, z = 0.f;
    } velocity;

    std::optional<EntityUpdate> currentUpdate;
    std::optional<AnimationState> currentAnimation;
    OverlayStack overlays;

    std::optional<float> gravityOverride;
    std::optional<float> maxFallSpeedOverride;
    std::optional<bool> canMoveWhileFallingOverride;
};

struct AnimationEffect {
    int id;
    XYVectorf pos;
    float heightStart = 0.f;
    float height = 64.f; // world units tall
    float width = 64.f;  // world units wide
    float scaleX = 1.f;  // texture scale within width (1 = fit exactly)
    float scaleY = 1.f;  // texture scale within height (1 = fit exactly)
    AnimationState animState;
};

} // namespace TwoHalfD

#endif
