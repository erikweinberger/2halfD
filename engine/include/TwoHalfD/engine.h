#ifndef ENGINE_H
#define ENGINE_H

#include <SFML/Graphics.hpp>
#include <cmath>
#include <iostream>
#include <numbers>
#include <span>
#include <unordered_map>
#include <vector>

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
    Position m_cameraPos;
    float m_cameraHeight;
    std::array<Event, 512> m_inputArray{};
    int m_currentInput{0};

    // RENDERCOMPONENTS
    sf::RenderWindow m_window;
    sf::RenderWindow m_window_above;
    sf::RenderTexture m_renderTexture;
    std::unordered_map<int, sf::Texture> m_textures;

  public:
    Engine() = default;

    Engine(const EngineSettings &engineSettings)
        : m_engineSettings(engineSettings), m_engineState(EngineState::None), m_engineContext(), m_level(), m_cameraPos(),
          m_window(sf::VideoMode(engineSettings.windowDim.x, engineSettings.windowDim.y), "Two Half D"),
          m_window_above(sf::VideoMode(1920, 1080), "Mini Map")
    {
        m_renderTexture.create(engineSettings.resolution.x, engineSettings.resolution.y);
        m_engineState = EngineState::initlised;
    }

    void loadLevel(const Level &level);
    EngineState getState();
    void setState(TwoHalfD::EngineState newState);

    std::span<const TwoHalfD::Event> getFrameInputs();
    void clearFrameInputs();
    void backgroundFrameUpdates();
    XYVector getMouseDeltaFrame();

    TwoHalfD::Position getCameraPosition();
    void setCameraPosition(const Position &newPos);
    void updateCameraPosition(const Position &posUpdate);
    WindowDim getWindowDimension();

    // RENDER FUNCTIONS
    void renderAbove();
    void render();
    void renderWalls();
    void renderFloor();
};
} // namespace TwoHalfD

#endif
