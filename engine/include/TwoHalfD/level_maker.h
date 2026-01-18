#ifndef LEVEL_MAKER_H
#define LEVEL_MAKER_H

#include "TwoHalfD/engine_types.h"

#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Vector2.hpp>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>

namespace fs = std::filesystem;

namespace TwoHalfD {

enum EntityTypes {
    texture = 0,
    wall,
    sprite
};

class LevelMaker {
  private:
    int m_entityId;
    std::vector<TwoHalfD::Wall> m_walls;
    std::vector<TwoHalfD::SpriteEntity> m_spriteEntities;
    std::unordered_map<int, TwoHalfD::TextureSignature> m_textures;
    const std::string m_defaultTextureFilePath = fs::path(ASSETS_DIR) / "textures/pattern_18_debug.png";

  public:
    LevelMaker() : m_entityId(0){};

    TwoHalfD::Level parseLevelFile(std::string levelFilePath);

    TwoHalfD::TextureSignature _makeTexture(std::string textureString);
    TwoHalfD::Wall _makeWall(std::string wallString);
    TwoHalfD::SpriteEntity _makeSpriteEntity(std::string spriteString);
};
} // namespace TwoHalfD

#endif