#ifndef ENGINE_H
#define ENGINE_H

#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <span>

namespace TwoHalfD {
using ObjectId = std::uint64_t;

struct Position {
    union {
        struct { float x, y; } pos;
        sf::Vector2f posf;
    } u = { {50.f, 400.f} };
    float direction = 0.f;
    
    Position() = default;
};

struct Wall {
    int id;
    sf::Vector2f start, end;
    int textureId;
};
struct SpriteEntity {
    int id;
    TwoHalfD::Position pos;
    int textureId;
    float scale;
};

struct EngineSettings {
  sf::Vector2u windowDim = {1920, 1080};
  sf::Vector2u resolution = {960, 540};
  float aspectRatio = 16.f / 9.f;
  float fov = 60.f;

  EngineSettings() = default;
};

struct Level {
  std::vector<Wall> walls;
  std::vector<SpriteEntity> sprites;
};

struct Event {
    enum class Type {
        None,
        KeyPressed,
        KeyReleased,
        MouseMoved,
        MouseButtonPressed,
        MouseButtonReleased,
    };

    Type type{Type::None};

    struct KeyEvent {
        int keyCode;
        int x, y;
    };

    struct MouseMoveEvent {
        int x, y;
    };

    struct MouseButtonEvent {
        int button;
        int x, y;
    };

    // Union of possible event payloads
    union {
        KeyEvent key;
        MouseMoveEvent mouseMove;
        MouseButtonEvent mouseButton;
    };

    Event() : type(Type::None) {}

    static Event KeyPressed(int keyCode, int x = 0, int y = 0) {
        Event e;
        e.type = Type::KeyPressed;
        e.key = { keyCode, x, y };
        return e;
    }

    static Event KeyReleased(int keyCode, int x = 0, int y = 0) {
        Event e;
        e.type = Type::KeyReleased;
        e.key = { keyCode, x, y };
        return e;
    }

    static Event MouseMoved(int x, int y) {
        Event e;
        e.type = Type::MouseMoved;
        e.mouseMove = { x, y };
        return e;
    }

    static Event MouseButtonPressed(int button, int x, int y) {
        Event e;
        e.type = Type::MouseButtonPressed;
        e.mouseButton = { button, x, y };
        return e;
    }

    static Event MouseButtonReleased(int button, int x, int y) {
        Event e;
        e.type = Type::MouseButtonReleased;
        e.mouseButton = { button, x, y };
        return e;
    }

};

enum class EngineState {
    None,
    initlised,
    running,
    ended
};

class Engine {

private:
    EngineSettings m_engineSettings;
    EngineState m_engineState; 
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
    // void update(float dt);
    // std::vector<Event> getEvents();

    // RENDER FUNCTIONS
    void render();
    void renderFloorCeil();

};
} // namespace TwoHalfD


#endif
