#ifndef ENGINE_TYPES
#define ENGINE_TYPES

// Sub-headers — include these directly for finer-grained dependencies
#include "TwoHalfD/types/bsp_types.h"
#include "TwoHalfD/types/entity_types.h"
#include "TwoHalfD/types/input_types.h"
#include "TwoHalfD/types/math_types.h"

#include <SFML/Graphics.hpp>
#include <cstddef>
#include <numbers>
#include <unordered_map>
#include <vector>

namespace TwoHalfD {

using ObjectId = std::uint64_t;

struct EngineSettings {
    sf::Vector2i windowDim = {960, 540};
    sf::Vector2i resolution = {960, 540};
    float aspectRatio = 16.f / 9.f;
    float fov = std::numbers::pi_v<float> / 3.f;
    float fovScale = std::tan(fov / 2);

    double graphicsFpsCap = 1000.0;
    double gameFpsCap = 60.0;

    float shaderScale = 256.f;

    bool cameraCollision = true;
    float heightClipping = 10.f; // How much difference in floor height is allowed before clipping occurs

    EngineSettings() = default;
};

struct Level {
    std::vector<Wall> walls;
    std::vector<SpriteEntity> sprites;
    std::unordered_map<int, TwoHalfD::TextureSignature> textures;
    std::unordered_map<int, FloorSection> floorSections;

    float cameraHeightStart;
    int seed = -1; // BSP seed, -1 means auto find best seed

    int defaultFloorTextureId = -1;
    XYVectorf defaultFloorStart;
    float defaultFloorHeight = 0.f;
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

    float cameraHeightStart{0.0f};
};

struct RenderZBuffer {
    std::vector<float> nearestWallRayDist;
};

enum class EngineState {
    None,
    initialised,
    running,
    fpsState,
    ended,
    paused
};

} // namespace TwoHalfD

#endif
