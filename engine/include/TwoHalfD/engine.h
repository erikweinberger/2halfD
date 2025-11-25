#ifndef ENGINE_H
#define ENGINE_H

#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <cmath>
#include <iostream>
#include <numbers>
#include <span>
#include <unordered_map>
#include <vector>

#include "engine_clocks.h"
#include "engine_types.h"

namespace TwoHalfD
{

class Engine
{

  private:
    EngineSettings m_engineSettings;
    EngineState m_engineState;
    EngineContext m_engineContext;
    Level m_level;
    CameraObject m_cameraObject;

    EngineClocks m_engineClocks;

    std::array<Event, 512> m_inputArray{};
    int m_currentInput{0};

    // RENDERCOMPONENTS
    sf::RenderWindow m_window;
    // sf::RenderWindow m_window_above;
    sf::RenderTexture m_renderTexture;
    TwoHalfD::RenderZBuffer m_renderZBuffer{};
    std::unordered_map<int, sf::Texture> m_textures;

  public:
    Engine() = default;

    Engine(const EngineSettings &engineSettings)
        : m_engineSettings(engineSettings), m_engineState(EngineState::None), m_engineContext(), m_level(), m_cameraObject(),
          m_engineClocks(EngineClocks{m_engineSettings.graphicsFpsCap, m_engineSettings.gameFpsCap}),
          m_window(sf::VideoMode(engineSettings.windowDim.x, engineSettings.windowDim.y), "Two Half D"),
          // m_window_above(sf::VideoMode(1, 1), "Mini Map"),
          m_renderZBuffer(TwoHalfD::RenderZBuffer{std::vector<float>(m_engineSettings.numRays, 0)})
    {
        m_renderTexture.create(engineSettings.resolution.x, engineSettings.resolution.y);
        m_engineState = EngineState::initlised;
    }

    void loadLevel(const Level &level);
    EngineState getState();
    void setState(TwoHalfD::EngineState newState);

    bool gameDeltaTimePassed();

    std::span<const TwoHalfD::Event> getFrameInputs();
    void clearFrameInputs();
    void backgroundFrameUpdates();

    // Getters and setters
    XYVector getMouseDeltaFrame();
    TwoHalfD::Position getCameraPosition();
    void setCameraPosition(const Position &newPos);
    void updateCameraPosition(const Position &posUpdate);
    XYVector getWindowDimension();

    std::vector<TwoHalfD::SpriteEntity> &getSpriteEntitiesInRegion();
    std::vector<TwoHalfD::SpriteEntity> &getAllSpriteEntities();
    std::vector<TwoHalfD::Wall> &getWallsInRegion();
    std::vector<TwoHalfD::Wall> &getAllWalls();

    // RENDER FUNCTIONS
    void renderAbove();
    void render();
    void renderOverlays();
    void renderObjects();
    void renderWalls();
    void renderFloor();
};
} // namespace TwoHalfD

#endif
