#ifndef RENDERER_H
#define RENDERER_H

#include <SFML/Graphics.hpp>
#include <filesystem>

#include "TwoHalfD/bsp/bsp_manager.h"
#include "TwoHalfD/engine_clocks.h"
#include "TwoHalfD/engine_types.h"
#include "TwoHalfD/entity_manager.h"

namespace TwoHalfD {

namespace fs = std::filesystem;

class Renderer {
  public:
    Renderer(sf::RenderWindow &window, const EngineSettings &settings, EngineClocks &clocks);

    void setData(const std::unordered_map<int, TextureSignature> *textures, const EntityManager *entityManager, float defaultFloorHeight,
                 int defaultFloorTextureId, XYVectorf defaultFloorStart);

    void render(const CameraObject &camera, BSPManager &bsp);

  private:
    sf::RenderWindow &m_window;
    const EngineSettings &m_settings;
    EngineClocks &m_clocks;

    sf::RenderTexture m_renderTexture;
    sf::Shader m_perspectiveShader;
    sf::Shader m_floorShader;
    RenderZBuffer m_renderZBuffer{};

    // Data sources (non-owning)
    const std::unordered_map<int, TextureSignature> *m_textures = nullptr;
    const EntityManager *m_entityManager = nullptr;
    float m_defaultFloorHeight = 0.f;
    int m_defaultFloorTextureId = -1;
    XYVectorf m_defaultFloorStart{};

    void renderBSP(const CameraObject &camera, BSPManager &bsp);
    void renderSegment(Segment segment, const CameraObject &camera);
    void renderSprite(const SpriteEntity &spriteEntity, const CameraObject &camera);
    void renderFloorSection(const FloorSection *floorSection, const CameraObject &camera);
    void renderFloor(const CameraObject &camera);
    void renderOverlays(const CameraObject &camera);
};

} // namespace TwoHalfD

#endif
