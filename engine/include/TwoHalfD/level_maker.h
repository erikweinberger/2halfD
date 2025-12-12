#include "engine_types.h"
#include <string>
#include <string_view>
#include <unordered_map>

namespace TwoHalfD
{

enum EntityTypes
{
    texture = 0,
    wall,
    sprite
};

class LevelMaker
{
  private:
    int m_entityId;
    std::vector<TwoHalfD::Wall> m_walls;
    std::vector<TwoHalfD::SpriteEntity> m_spriteEntities;
    std::unordered_map<int, TwoHalfD::TextureSignature> m_textures;
    const std::string m_defaultTextureFilePath = "../assets/textures/pattern_18_debug.png";

  public:
    LevelMaker() : m_entityId(0){};

    TwoHalfD::Level parseLevelFile(std::string levelFilePath);

    TwoHalfD::TextureSignature _makeTexture(std::string textureString);
    TwoHalfD::Wall _makeWall(std::string wallString);
    TwoHalfD::SpriteEntity _makeSpriteEntity(std::string spriteString);
};
} // namespace TwoHalfD