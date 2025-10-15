#ifndef ENGINE_TYPES
#define ENGINE_TYPES


#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <span>

#include<thread>
#include<chrono>


namespace TwoHalfD {

using ObjectId = std::uint64_t;

struct WindowDim { int x, y; };
struct XYVector { 
    int x, y;
    
    XYVector operator- (const XYVector &other) {
        XYVector result {this->x - other.x, this->y - other.y};
        return result;
    }

    XYVector operator+ (const XYVector &other) {
        XYVector result {this->x + other.x, this->y + other.y};
        return result;
    }
};

struct Position {
    struct xyCord { float x, y; };

    union {
        struct xyCord pos;
        sf::Vector2f posf;
    };
    float direction = 0.f;

    Position(float x = 0.f, float y = 0.f, float dir = 0.f) : 
        pos{x, y}, direction(dir) {}

    Position(sf::Vector2f v, float dir = 0.f) :
        posf(v), direction(dir) {}

    Position operator+(const Position& other) const {
        return Position(
            pos.x + other.pos.x,
            pos.y + other.pos.x,
            direction + other.direction
        );
    }

    Position& operator+=(const Position& other) {
        if (other.direction != 0.f) {
            std::this_thread::sleep_for(std::chrono::seconds(3));
            std::cerr << "Position direction other: " << other.direction << '\n';
        }
        pos.x += other.pos.x;
        pos.y += other.pos.y;
        direction += other.direction;
        return *this;
    }

    Position operator-(const Position& other) const {
        return Position(
            pos.x - other.pos.x,
            pos.y - other.pos.x,
            direction - other.direction
        );
    }

    Position& operator-=(const Position& other) {
        pos.x -= other.pos.x;
        pos.y -= other.pos.y;
        direction -= other.direction;
        return *this;
    }

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
  sf::Vector2i windowDim = {1920, 1080};
  sf::Vector2i resolution = {960, 540};
  float aspectRatio = 16.f / 9.f;
  float fov = 60.f;

  EngineSettings() = default;
};

struct Level {
  std::vector<Wall> walls;
  std::vector<SpriteEntity> sprites;
};

struct EngineContext {
    XYVector prevMousePosition = {0 , 0};
    XYVector currentMousePosition = {0 , 0};
    XYVector MouseDelta = {0 , 0};
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
        XYVector moveDelta;
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

    static Event MouseMoved(int x, int y, XYVector moveDelta) {
        Event e;
        e.type = Type::MouseMoved;
        e.mouseMove = { x, y, moveDelta };
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

enum keyCodeEnum {
    a = 0, b, c, d, e, f, g, h, i, j, k, l, m,
    n, o, p, q, r, s, t, u, v, w, x, y, z
};

enum class EngineState {
    None,
    initlised,
    running,
    fpsState,
    ended
};
}

#endif


