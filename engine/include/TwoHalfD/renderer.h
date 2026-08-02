#ifndef RENDERER_H
#define RENDERER_H

#include <SFML/Graphics.hpp>
#include <filesystem>
#include <optional>
#include <sys/_types/_u_int.h>
#include <sys/_types/_u_int32_t.h>

#include "SFML/Graphics/Color.hpp"
#include "SFML/Graphics/Font.hpp"
#include "TwoHalfD/bsp/bsp_manager.h"
#include "TwoHalfD/engine_clocks.h"
#include "TwoHalfD/engine_types.h"
#include "TwoHalfD/entity_manager.h"

namespace TwoHalfD {

namespace fs = std::filesystem;

struct DebugSettings {
    sf::Font debugFont;
    sf::Color debugColor;
    size_t fontSize;

    u_int32_t maxFontSize = 28;
    u_int32_t minFontSize = 12;
    u_int32_t numSpritesRenderedFrame = 0u;
    u_int32_t numSegmentsRenderedFrame = 0u;
    u_int32_t numFloorSectionsRenderedFrame = 0u;
    u_int32_t numColourOverlaysRenderedFrame = 0u;
    u_int32_t numEffectsRenderedFrame = 0u;

    std::optional<sf::Text> cachedText;
    sf::Clock updateTimer;
    float updateInterval = 0.5f;
    bool needsUpdate = true;
};

class Renderer {
  public:
    Renderer(sf::RenderWindow &window, const TwoHalfD::EngineSettings &settings, TwoHalfD::EngineClocks &clocks);

    void setData(const std::unordered_map<int, TextureSignature> *textures, const TwoHalfD::EntityManager *entityManager, float defaultFloorHeight,
                 int defaultFloorTextureId, TwoHalfD::XYVectorf defaultFloorStart);

    void render(const TwoHalfD::CameraObject &camera, TwoHalfD::BSPManager &bsp);

  private:
    sf::RenderWindow &m_window;
    const EngineSettings &m_settings;
    EngineClocks &m_clocks;

    sf::RenderTexture m_renderTexture;
    sf::Shader m_perspectiveShader;
    sf::Shader m_floorShader;
    // Data sources (non-owning)
    const std::unordered_map<int, TextureSignature> *m_textures = nullptr;
    const TwoHalfD::EntityManager *m_entityManager = nullptr;
    float m_defaultFloorHeight = 0.f;
    int m_defaultFloorTextureId = -1;
    TwoHalfD::XYVectorf m_defaultFloorStart{};

    TwoHalfD::DebugSettings m_debugSettings;

    void renderBSP(const TwoHalfD::CameraObject &camera, TwoHalfD::BSPManager &bsp);
    void renderSegment(const TwoHalfD::Segment &segment, const TwoHalfD::CameraObject &camera);
    void renderSprite(const TwoHalfD::SpriteEntity &spriteEntity, const TwoHalfD::CameraObject &camera);
    void renderEffect(const TwoHalfD::AnimationEffect &effect, const TwoHalfD::CameraObject &camera);
    void renderFloorSection(const TwoHalfD::FloorSection *floorSection, const TwoHalfD::CameraObject &camera);
    void renderColourOverlay(const TwoHalfD::FloorColourOverlay *overlay, const TwoHalfD::CameraObject &camera);
    void renderFloor(const TwoHalfD::CameraObject &camera);
    void renderOverlays(const TwoHalfD::CameraObject &camera);
    void renderDebugOverlays(const TwoHalfD::CameraObject &camera, const std::optional<TwoHalfD::SpriteEntity> &spriteEntity,
                             const TwoHalfD::BSPManager &bsp);
};

} // namespace TwoHalfD

#endif
