#ifndef ENTITY_TYPES_H
#define ENTITY_TYPES_H

#include "TwoHalfD/types/animation_types.h"
#include "TwoHalfD/types/math_types.h"

#include <SFML/Graphics/Texture.hpp>
#include <cstddef>
#include <optional>
#include <string>
#include <variant>
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
};

struct FloorSection {
    Polygon vertices;
    XYVectorf floorTextureStart;
    int id;
    int textureId;
    float height;
    bool isCCW;
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

struct SpriteEntity {
    int id;
    TwoHalfD::Position pos;
    float radius;
    int height;
    int textureId;
    float scale;
    float heightStart = 0.f;
    float speed = 5.f;

    std::optional<EntityUpdate> currentUpdate;
    std::optional<AnimationState> currentAnimation;
    OverlayStack overlays;
};

struct AnimationEffect {
    int id;
    XYVectorf pos;
    float heightStart = 0.f;
    float height = 64.f;   // world units tall
    float width = 64.f;    // world units wide
    float scaleX = 1.f;    // texture scale within width (1 = fit exactly)
    float scaleY = 1.f;    // texture scale within height (1 = fit exactly)
    AnimationState animState;
};

} // namespace TwoHalfD

#endif
