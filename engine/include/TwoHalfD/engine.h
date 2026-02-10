#ifndef ENGINE_H
#define ENGINE_H

#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <numbers>
#include <span>
#include <unordered_map>
#include <vector>

#include "TwoHalfD/bsp/bsp_manager.h"
#include "engine_clocks.h"
#include "engine_types.h"
#include "level_maker.h"

namespace TwoHalfD {

namespace fs = std::filesystem;

class Engine {

  private:
    EngineSettings m_engineSettings;
    EngineState m_engineState;
    EngineContext m_engineContext;

    Level m_level;
    TwoHalfD::LevelMaker m_levelMaker;
    CameraObject m_cameraObject;

    EngineClocks m_engineClocks;

    std::array<Event, 512> m_inputArray{};
    int m_currentInput{0};

    // RENDER COMPONENTS
    sf::RenderWindow m_window;
    sf::RenderWindow m_window_above;
    sf::RenderTexture m_renderTexture;
    TwoHalfD::RenderZBuffer m_renderZBuffer{};
    std::unordered_map<int, sf::Texture> m_textures;
    TwoHalfD::BSPManager m_bspManager;
    sf::Shader m_perspectiveShader;
    sf::Shader m_floorShader;

    std::chrono::steady_clock::time_point m_lastFrameTime;
    std::chrono::steady_clock::time_point m_fpsUpdateTime;
    int m_frameCount = 0;
    float m_currentFPS = 0.0f;

  public:
    Engine() = default;

    Engine(const EngineSettings &engineSettings)
        : m_engineSettings(engineSettings), m_engineState(EngineState::None), m_engineContext(), m_level(), m_cameraObject(),
          m_engineClocks(EngineClocks{m_engineSettings.graphicsFpsCap, m_engineSettings.gameFpsCap}),
          m_window(sf::VideoMode(engineSettings.windowDim.x, engineSettings.windowDim.y), "Two Half D"),
          // m_window_above(sf::VideoMode(800, 800), "Mini Map"),
          m_renderZBuffer(TwoHalfD::RenderZBuffer{std::vector<float>(m_engineSettings.numRays, 0)}) {

        m_window.setVerticalSyncEnabled(false);
        m_window.setFramerateLimit(0);

        m_renderTexture.create(engineSettings.resolution.x, engineSettings.resolution.y);
        m_engineState = EngineState::initialised;
        if (!sf::Shader::isAvailable()) {
            std::cerr << "Shaders not available!" << std::endl;
        }
        // Load shader (you can also loadFromMemory)
        std::string shadersPath = static_cast<std::string>(ROOT_DIR) + "/engine/include/TwoHalfD/" + "shaders/perspectiveShader2.frag";
        if (!m_perspectiveShader.loadFromFile(shadersPath, sf::Shader::Fragment)) {
            std::cerr << "Failed to load shader!" << std::endl;
            std::exit(1);
        }
        // Load floor shader
        std::string floorShaderPath = static_cast<std::string>(ROOT_DIR) + "/engine/include/TwoHalfD/" + "shaders/floorShader.frag";
        if (!m_floorShader.loadFromFile(floorShaderPath, sf::Shader::Fragment)) {
            std::cerr << "Failed to load floor shader!" << std::endl;
            std::exit(1);
        }
    }

    void loadLevel(const std::string levelFilePath);
    EngineState getState();
    void setState(TwoHalfD::EngineState newState);

    bool gameDeltaTimePassed();

    std::span<const TwoHalfD::Event> getFrameInputs();
    void clearFrameInputs();
    void backgroundFrameUpdates();

    // GETTERS AND SETTERS
    XYVector getMouseDeltaFrame();
    TwoHalfD::Position getCameraPosition();
    void setCameraPosition(const Position &newPos);
    TwoHalfD::Position updateCameraPosition(const Position &posUpdate);
    XYVector getWindowDimension();

    std::vector<TwoHalfD::SpriteEntity> &getSpriteEntitiesInRegion();
    std::vector<TwoHalfD::SpriteEntity> &getAllSpriteEntities();
    std::vector<TwoHalfD::Wall> &getWallsInRegion();
    std::vector<TwoHalfD::Wall> &getAllWalls();

    // RENDER FUNCTIONS
    // void renderAbove();
    void render();
    void renderOverlays();
    void renderObjects();
    void renderWalls();
    void renderSegment(TwoHalfD::Segment segment);
    void renderFloor();
    void renderFloor2();
    void renderSprite(const TwoHalfD::SpriteEntity &spriteEntity);

    // PHYSICS FUNCTIONS

    std::pair<const Wall *, float> findNearestWall(const XYVectorf &cord, const TwoHalfD::XYVectorf &rayDir, const std::vector<Wall> &walls);
    std::pair<const Wall *, float> findNearestWall(const Position &ray);
    std::pair<const Wall *, float> findNearestWall(const Position &ray, const std::vector<Wall> &walls);

    const std::vector<const Wall *> wallCollisionSelf(const CameraObject &cameraObject);
    const std::vector<const Wall *> wallCollisionSelf();
    const Wall &wallCollisionSprite(const SpriteEntity &spriteEntity);
};
} // namespace TwoHalfD

#endif
