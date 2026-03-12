#ifndef ENTITY_TYPES_H
#define ENTITY_TYPES_H

#include "TwoHalfD/types/math_types.h"

#include <SFML/Graphics/Texture.hpp>
#include <string>
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

struct SpriteEntity {
    int id;
    TwoHalfD::Position pos;
    int radius;
    int height;
    int textureId;
    float scale;
    float heightStart = 0.f;
};

struct FloorSection {
    Polygon vertices;
    XYVectorf floorTextureStart;
    int id;
    int textureId;
    float height;
    bool isCCW;
};

} // namespace TwoHalfD

#endif
