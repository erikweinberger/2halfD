#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <vector>
#include <iostream>

namespace TwoHalfD {
using ObjectId = std::uint64_t;

struct Position {
    sf::Vector2f pos = {50, 400};
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

/*struct Event {
    enum class Type {
        None,
        KeyPressed,
        KeyReleased,
        MouseMoved,
        MouseButtonPressed,
        MouseButtonReleased,
    };
};*/

enum class EngineState {
    None,
    initlised,
    running,
    ended
};

class Engine {
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
    // void update(float dt);
    // std::vector<Event> getEvents();

    // RENDER FUNCTIONS
    void render();
    void renderFloorCeil();

private:
    EngineSettings m_engineSettings;
    EngineState m_engineState; 
    Level m_level;
    Position m_cameraPos;

    // RENDERCOMPONENTS
    sf::RenderWindow m_window;
    sf::RenderTexture m_renderTexture;
};
} // namespace TwoHalfD
