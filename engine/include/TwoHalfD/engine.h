#ifndef ENGINE_H
#define ENGINE_H

#include <SFML/Graphics.hpp>
#include <span>

#include "TwoHalfD/bsp/bsp_manager.h"
#include "TwoHalfD/engine_clocks.h"
#include "TwoHalfD/engine_types.h"
#include "TwoHalfD/entity_manager.h"
#include "TwoHalfD/input_manager.h"
#include "TwoHalfD/level_maker.h"
#include "TwoHalfD/renderer.h"

namespace TwoHalfD {

class Engine {

  private:
    EngineSettings m_engineSettings;
    EngineState m_engineState;

    TwoHalfD::LevelMaker m_levelMaker;
    CameraObject m_cameraObject;

    // Level data owned by Engine after initialization
    std::unordered_map<int, TextureSignature> m_textures;
    float m_defaultFloorHeight = 0.f;
    int m_defaultFloorTextureId = -1;
    XYVectorf m_defaultFloorStart{};

    EngineClocks m_engineClocks;

    sf::RenderWindow m_window;
    TwoHalfD::BSPManager m_bspManager;
    TwoHalfD::Renderer m_renderer;
    TwoHalfD::InputManager m_inputManager;
    TwoHalfD::EntityManager m_entityManager;

    void backgroundFrameUpdates();

  public:
    Engine(const EngineSettings &engineSettings)
        : m_engineSettings(engineSettings), m_engineState(EngineState::None), m_cameraObject(),
          m_engineClocks(EngineClocks{m_engineSettings.graphicsFpsCap, m_engineSettings.gameFpsCap}),
          m_window(sf::VideoMode(engineSettings.windowDim.x, engineSettings.windowDim.y), "Two Half D"),
          m_renderer(m_window, m_engineSettings, m_engineClocks), m_inputManager(m_window, m_engineSettings) {

        m_window.setVerticalSyncEnabled(false);
        m_window.setFramerateLimit(0);
        m_engineState = EngineState::initialised;
    }

    void loadLevel(const std::string levelFilePath);
    EngineState getState();
    void setState(TwoHalfD::EngineState newState);
    bool gameDeltaTimePassed();

    std::span<const TwoHalfD::Event> getFrameInputs();
    void clearFrameInputs();

    TwoHalfD::Position getCameraPosition();
    void setCameraPosition(const Position &newPos);
    TwoHalfD::Position updateCameraPosition(const Position &posUpdate);

    const std::vector<TwoHalfD::Wall> &getAllWalls();
    const std::unordered_map<int, TwoHalfD::SpriteEntity> &getAllSpriteEntities();
    TwoHalfD::EntityManager &getEntityManager();
    void walkTo(int entityId, TwoHalfD::XYVectorf targetPos, float maxHeightDiff = 0.f, float maxDistance = 10000.f);
    std::vector<TwoHalfD::XYVectorf> getPathfindingPoints(TwoHalfD::XYVectorf start, TwoHalfD::XYVectorf end, float entityWidth, float maxHeightDiff,
                                                          float maxDistance);

    void render();
};

} // namespace TwoHalfD

#endif
