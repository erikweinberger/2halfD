#include "game.h"
#include "TwoHalfD/engine.h"
#include "TwoHalfD/engine_types.h"
#include <cassert>
#include <chrono>
#include <filesystem>
#include <numbers>

namespace fs = std::filesystem;

static constexpr int OVERLAY_ID = 1;
static const TwoHalfD::Polygon OVERLAY_POLYGON = {
    {100.f, 100.f}, {400.f, 100.f}, {400.f, 250.f},
    {250.f, 250.f}, {250.f, 500.f}, {100.f, 500.f}};

void Game::run() {
    fs::path levelFile = fs::path(ASSETS_DIR) / "levels" / "level1.txt";
    m_engine.loadLevel(levelFile);
    m_engine.addColourOverlay(OVERLAY_ID, OVERLAY_POLYGON, 0.f, 255, 0, 0, 128);
    while (m_engine.getState() == TwoHalfD::EngineState::running || m_engine.getState() == TwoHalfD::EngineState::fpsState ||
           m_engine.getState() == TwoHalfD::EngineState::paused) {
        if (m_engine.gameDeltaTimePassed()) {
            handleFrameInputs();
            updateGameState();
        }
        m_engine.render();
    }
}

// Game Logic
int frameCount = 0;
bool overlayVisible = true;
auto overlayToggleTime = std::chrono::steady_clock::now();

void Game::updateGameState() {
    auto now = std::chrono::steady_clock::now();
    float elapsed = std::chrono::duration<float>(now - overlayToggleTime).count();
    if (elapsed >= 2.f) {
        overlayToggleTime = now;
        overlayVisible = !overlayVisible;
        if (overlayVisible)
            m_engine.addColourOverlay(OVERLAY_ID, OVERLAY_POLYGON, 0.f, 255, 0, 0, 128);
        else
            m_engine.removeColourOverlay(OVERLAY_ID);
    }

    float x = 0.f, y = 0.f;

    const auto &moveDir = m_gameState.playerState.moveDir;
    const float playerDir = m_gameState.playerState.playerPos.direction;
    int forward = (moveDir.w - moveDir.s);
    int strafe = (moveDir.d - moveDir.a);

    x = forward * std::cosf(playerDir) - strafe * std::sinf(playerDir);
    y = forward * std::sinf(playerDir) + strafe * std::cosf(playerDir);
    float length = std::sqrt(x * x + y * y);
    if (length > 0.f) {
        x /= length;
        y /= length;
    }
    x *= 10;
    y *= 10;
    TwoHalfD::Position moveVector{x, y, 0.f};
    m_gameState.playerState.playerPos += moveVector;

    m_gameState.playerState.playerPos = m_engine.updateCameraPosition(moveVector);

    auto sprites = m_engine.getAllSpriteEntities();

    for (const auto &[id, entity] : sprites) {
        if (frameCount % 60 == 0) {
            m_engine.walkTo(entity.id, m_gameState.playerState.playerPos.pos, 25.f, 10000.f);
            m_engine.setAnimation(entity.id, 1, true);
        } else if (!entity.currentUpdate) {
            m_engine.clearAnimation(entity.id);
        }

        if (frameCount % 180 == 0) {
            m_engine.addOverlay(entity.id, 3, 0.4f, 0.3f, 45, 45, 1, false, 1.f, 1.f);
        }
    }

    if (frameCount % 300 == 0) {
        float dir = m_gameState.playerState.playerPos.direction;
        TwoHalfD::XYVectorf effectPos = {m_gameState.playerState.playerPos.pos.x + 500.f * std::cos(dir),
                                         m_gameState.playerState.playerPos.pos.y + 500.f * std::sin(dir)};
        // m_engine.spawnEffect(effectPos, /*templateId=*/2, /*height=*/60.f, /*width=*/60.f, /*scaleX=*/1.f, /*scaleY=*/0.5f, /*heightStart=*/30.f);
    }

    frameCount++;
}

// Input / Events
void Game::handleFrameInputs() {
    std::span<const TwoHalfD::Event> inputs = m_engine.getFrameInputs();
    for (auto &input : inputs) {
        switch (input.type) {
        case TwoHalfD::Event::Type::None:
            break;
        case TwoHalfD::Event::Type::KeyPressed:
            handleKeyPressedEvent(input);
            break;
        case TwoHalfD::Event::Type::KeyReleased:
            handleKeyReleasedEvent(input);
            break;
        case TwoHalfD::Event::Type::MouseMoved:
            handleMouseMoveEvent(input);
            break;
        default:
            break;
        }
    }
    m_engine.clearFrameInputs();
}

void Game::handleKeyPressedEvent(const TwoHalfD::Event &event) {
    assert(event.type == TwoHalfD::Event::Type::KeyPressed);
    switch (event.key.keyCode) {
    case TwoHalfD::w:
        m_gameState.playerState.moveDir.w = 1;
        break;
    case TwoHalfD::a:
        m_gameState.playerState.moveDir.a = 1;
        break;
    case TwoHalfD::s:
        m_gameState.playerState.moveDir.s = 1;
        break;
    case TwoHalfD::d:
        m_gameState.playerState.moveDir.d = 1;
        break;
    case TwoHalfD::p:
        if (m_engine.getState() == TwoHalfD::EngineState::paused) m_engine.setState(TwoHalfD::EngineState::fpsState);
        else m_engine.setState(TwoHalfD::EngineState::paused);
        break;
    default:
        break;
    }
}

void Game::handleKeyReleasedEvent(const TwoHalfD::Event &event) {
    assert(event.type == TwoHalfD::Event::Type::KeyReleased);
    switch (event.key.keyCode) {
    case TwoHalfD::w:
        m_gameState.playerState.moveDir.w = 0;
        break;
    case TwoHalfD::a:
        m_gameState.playerState.moveDir.a = 0;
        break;
    case TwoHalfD::s:
        m_gameState.playerState.moveDir.s = 0;
        break;
    case TwoHalfD::d:
        m_gameState.playerState.moveDir.d = 0;
        break;
    default:
        break;
    }
}

void Game::handleMouseMoveEvent(const TwoHalfD::Event &event) {
    assert(event.type == TwoHalfD::Event::Type::MouseMoved);
    TwoHalfD::XYVector mouseDelta = event.mouseMove.moveDelta;
    float newAngle = m_gameState.playerState.playerPos.direction - (mouseDelta.x) / 200.f;
    newAngle = std::fmod(newAngle, 2 * std::numbers::pi_v<float>);
    if (newAngle < 0) newAngle += 2 * std::numbers::pi_v<float>;
    m_gameState.playerState.playerPos.direction = newAngle;
    m_engine.setCameraPosition(m_gameState.playerState.playerPos);
}

TwoHalfD::Position Game::showPosition() {
    return m_gameState.playerState.playerPos;
}