#ifndef RENDERER_H
#define RENDERER_H

#include <SFML/Graphics.hpp>
#include <filesystem>

#include "TwoHalfD/bsp/bsp_manager.h"
#include "TwoHalfD/engine_types.h"
#include "TwoHalfD/engine_clocks.h"

namespace TwoHalfD {

namespace fs = std::filesystem;

class Renderer {
  public:
    Renderer(sf::RenderWindow &window, const EngineSettings &settings, EngineClocks &clocks);

    void render(const CameraObject &camera, Level &level, BSPManager &bsp);

  private:
    sf::RenderWindow &m_window;
    const EngineSettings &m_settings;
    EngineClocks &m_clocks;

    sf::RenderTexture m_renderTexture;
    sf::Shader m_perspectiveShader;
    sf::Shader m_floorShader;
    RenderZBuffer m_renderZBuffer{};

    void renderBSP(const CameraObject &camera, Level &level, BSPManager &bsp);
    void renderSegment(Segment segment, const CameraObject &camera, const Level &level);
    void renderSprite(const SpriteEntity &spriteEntity, const CameraObject &camera, const Level &level);
    void renderFloorSection(const FloorSection *floorSection, const CameraObject &camera, const Level &level);
    void renderFloor(const CameraObject &camera, const Level &level);
    void renderOverlays(const CameraObject &camera);
};

} // namespace TwoHalfD

#endif
