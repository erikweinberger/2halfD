#ifndef ENGINE_TYPES
#define ENGINE_TYPES

#include <SFML/Graphics.hpp>
#include <cstddef>
#include <iostream>
#include <limits>
#include <numbers>
#include <optional>
#include <span>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <chrono>
#include <thread>

namespace TwoHalfD {

using ObjectId = std::uint64_t;

struct XYVector {
    int x, y;

    XYVector operator-(const XYVector &other) {
        XYVector result{this->x - other.x, this->y - other.y};
        return result;
    }

    XYVector operator+(const XYVector &other) {
        XYVector result{this->x + other.x, this->y + other.y};
        return result;
    }

    bool operator==(const XYVector &other) const {
        return this->x == other.x && this->y == other.y;
    }

    friend std::ostream &operator<<(std::ostream &os, const XYVector &v) {
        os << "(" << v.x << " , " << v.y << ")";
        return os;
    }
};

struct XYVectorf {
    float x, y;

    XYVectorf() : x(0), y(0) {}

    XYVectorf(float x, float y) : x(x), y(y) {}
    XYVectorf(const sf::Vector2f &v) : x(v.x), y(v.y) {}

    float length() const {
        return std::sqrt(x * x + y * y);
    }

    // Squared length (faster, avoids sqrt)
    float lengthSquared() const {
        return x * x + y * y;
    }

    // Normalize the vector (returns unit vector)
    XYVectorf normalized() const {
        float len = length();
        if (len > 0) {
            return XYVectorf(x / len, y / len);
        }
        return XYVectorf(0, 0);
    }

    XYVectorf operator-(const XYVectorf &other) const {
        XYVectorf result{this->x - other.x, this->y - other.y};
        return result;
    }

    XYVectorf operator+(const XYVectorf &other) const {
        XYVectorf result{this->x + other.x, this->y + other.y};
        return result;
    }

    XYVectorf operator*(const float scalar) const {
        return XYVectorf(this->x * scalar, this->y * scalar);
    }

    friend XYVectorf operator*(const float scalar, const XYVectorf &vec) {
        return XYVectorf(vec.x * scalar, vec.y * scalar);
    }

    bool operator==(const XYVectorf &other) const {
        return this->x == other.x && this->y == other.y;
    }

    friend std::ostream &operator<<(std::ostream &os, const XYVectorf &v) {
        os << "(" << v.x << " , " << v.y << ")";
        return os;
    }
};

inline float dot(const XYVectorf &a, const XYVectorf &b) {
    return a.x * b.x + a.y * b.y;
}

struct Position {

    union {
        XYVectorf pos;
        sf::Vector2f posf;
    };
    float direction = 0.f;

    Position(float x = 2 * 256.0f, float y = 3 * 256.0f, float dir = (3 * std::numbers::pi_v<float> / 2.0f)) : pos{x, y}, direction(dir) {}

    Position(sf::Vector2f v, float dir = 0.f) : posf(v), direction(dir) {}

    Position(XYVectorf v, float dir = 0.f) : pos(v), direction(dir) {}

    Position operator+(const Position &other) const {
        return Position(pos.x + other.pos.x, pos.y + other.pos.x, direction + other.direction);
    }

    Position &operator+=(const Position &other) {
        pos.x += other.pos.x;
        pos.y += other.pos.y;
        direction += other.direction;
        return *this;
    }

    Position operator-(const Position &other) const {
        return Position(pos.x - other.pos.x, pos.y - other.pos.x, direction - other.direction);
    }

    Position &operator-=(const Position &other) {
        pos.x -= other.pos.x;
        pos.y -= other.pos.y;
        direction -= other.direction;
        return *this;
    }
};

struct TextureSignature {
    int id;
    std::string filePath;
    sf::Texture texture;
};

struct Wall {
    int id;
    XYVectorf start, end;
    float height;
    int textureId;
};

struct SpriteEntity {
    int id;
    TwoHalfD::Position pos;
    int radius;
    int height;
    int textureId;
    float scale;
};

struct floorSection {
    int id;
    XYVectorf floorStart;
    std::vector<XYVectorf> vertices;
    int textureId;
};

struct EngineSettings {
    sf::Vector2i windowDim = {960, 540};
    sf::Vector2i resolution = {960, 540};
    float aspectRatio = 16.f / 9.f;
    float fov = std::numbers::pi_v<float> / 3.f;
    float fovScale = std::tan(fov / 2);
    int numRays = 960;

    double graphicsFpsCap = 1000.0;
    double gameFpsCap = 60.0;

    float shaderScale = 256.f;

    bool cameraCollision = true;

    EngineSettings() = default;
};

struct Level {
    std::vector<Wall> walls;
    std::vector<SpriteEntity> sprites;
    std::unordered_map<int, TwoHalfD::TextureSignature> textures;
    std::vector<floorSection> floorSections;

    float cameraHeightStart;
    int seed = -1; // BSP seed, -1 means auto find best seed

    int defaultFloorTextureId = -1;
    XYVectorf defaultFloorStart;
};

struct EngineContext {
    XYVector prevMousePosition = {0, 0};
    XYVector currentMousePosition = {0, 0};
    XYVector MouseDelta = {0, 0};
};

struct CameraObject {
    Position cameraPos{300, 700, 3 * std::numbers::pi_v<float> / 2};
    float cameraHeight{100.f};
    float cameraRadius{64};
};

struct RenderZBuffer {
    std::vector<float> nearestWallRayDist;
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
        e.key = {keyCode, x, y};
        return e;
    }

    static Event KeyReleased(int keyCode, int x = 0, int y = 0) {
        Event e;
        e.type = Type::KeyReleased;
        e.key = {keyCode, x, y};
        return e;
    }

    static Event MouseMoved(int x, int y, XYVector moveDelta) {
        Event e;
        e.type = Type::MouseMoved;
        e.mouseMove = {x, y, moveDelta};
        return e;
    }

    static Event MouseButtonPressed(int button, int x, int y) {
        Event e;
        e.type = Type::MouseButtonPressed;
        e.mouseButton = {button, x, y};
        return e;
    }

    static Event MouseButtonReleased(int button, int x, int y) {
        Event e;
        e.type = Type::MouseButtonReleased;
        e.mouseButton = {button, x, y};
        return e;
    }
};

enum keyCodeEnum {
    a = 0,
    b,
    c,
    d,
    e,
    f,
    g,
    h,
    i,
    j,
    k,
    l,
    m,
    n,
    o,
    p,
    q,
    r,
    s,
    t,
    u,
    v,
    w,
    x,
    y,
    z
};

enum class EngineState {
    None,
    initialised,
    running,
    fpsState,
    ended,
    paused
};

// BSP
struct Segment {
    XYVectorf v1;
    XYVectorf v2;
    const Wall *wall;
    float wallRatioStart{0.f};
    float wallRatioEnd{1.f};
};

struct BSPNode {
    std::unique_ptr<BSPNode> front;
    std::unique_ptr<BSPNode> back;

    std::unordered_set<int> spriteIds;

    XYVectorf splitterP0;
    XYVectorf splitterP1;
    XYVectorf splitterVec;

    int segmentID = -1;
};

struct DrawCommand {
    enum Type {
        Segment,
        Sprite
    } type;
    int id;
};

} // namespace TwoHalfD

#endif
