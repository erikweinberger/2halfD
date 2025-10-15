#ifndef ENGINE_H
#define ENGINE_H

#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <span>

#include "engine_types.h"

namespace TwoHalfD {

class Engine {

private:
    EngineSettings m_engineSettings;
    EngineState m_engineState; 
    EngineContext m_engineContext;
    Level m_level;
    Position m_cameraPos;
    std::array<Event, 512> m_inputArray{};
    int m_currentInput { 0 };

    // RENDERCOMPONENTS
    sf::RenderWindow m_window;
    sf::RenderTexture m_renderTexture;

public:
   Engine() = default;

    Engine(const EngineSettings &engineSettings) : 
        m_engineSettings(engineSettings),
        m_engineState(EngineState::None),
        m_engineContext(),
        m_level(),
        m_cameraPos(),
        m_window(sf::VideoMode(engineSettings.windowDim.x, engineSettings.windowDim.y), "Two Half D")
    {
        m_renderTexture.create(engineSettings.resolution.x, engineSettings.resolution.y);
        m_engineState = EngineState::initlised;

    }

    void loadLevel(const Level &level);
    EngineState getState();

    std::span<const TwoHalfD::Event> getFrameInputs();
    void clearFrameInputs();
    void backgroundFrameUpdates();
    XYVector getMouseDeltaFrame();

    void setCameraPosition(const Position & newPos);
    void updateCameraPosition(const Position &posUpdate);
    WindowDim getWindowDimension();

    // void update(float dt);
    // std::vector<Event> getEvents();

    // RENDER FUNCTIONS
    void render();
    void renderFloorCeil();

};
} // namespace TwoHalfD


#endif
