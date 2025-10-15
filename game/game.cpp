#include "game.h"
#include "TwoHalfD/engine_types.h"
#include <cassert>
#include <numbers>

void Game::run() {
    TwoHalfD::Level level;
    m_engine.loadLevel(level);
    while (m_engine.getState() == TwoHalfD::EngineState::running || m_engine.getState() == TwoHalfD::EngineState::fpsState) {
        handleFrameInputs();
        updateGameState();
        m_engine.render();
    }
}


// Game Logic
void Game::updateGameState() {
    float x = 0.f, y = 0.f;

    const auto& moveDir = m_gameState.playerState.moveDir;
    const float playerDir = m_gameState.playerState.playerPos.direction;
    int forward = (moveDir.w - moveDir.s);
    int strafe = (moveDir.d - moveDir.a);

    x = forward * std::cosf(playerDir) - strafe * std::sinf(playerDir);
    y = forward * std::sinf(playerDir) + strafe * std::cosf(playerDir);
    float length = std::sqrt(x*x + y*y);
    if (length > 0.f) {
        x /= length;
        y /= length;
    }

    TwoHalfD::Position moveVector{x, y, 0.f};
    m_gameState.playerState.playerPos += moveVector;


    m_engine.setCameraPosition(m_gameState.playerState.playerPos);
}


// Input / Events
void Game::handleFrameInputs() {
    std::span<const TwoHalfD::Event> inputs = m_engine.getFrameInputs();
    for (auto &input : inputs) {
        //std::cout << "Event type: " << static_cast<int>(input.type) << std::endl;
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
            default:
                break;

        }
    }   
    m_engine.clearFrameInputs();
}

void Game::handleKeyPressedEvent(const TwoHalfD::Event & event) {
    assert(event.type == TwoHalfD::Event::Type::KeyPressed);  
    // std::cout << "Key pressed: " << event.key.keyCode << "\n";
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
        default:
            break;
    }
}

void Game::handleKeyReleasedEvent(const TwoHalfD::Event & event) {
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


void Game::handleMouseMoveEvent(const TwoHalfD::Event & event) {
    assert(event.type == TwoHalfD::Event::Type::MouseMoved);
    if (event.mouseMove.x == 960 && event.mouseMove.y == 540) return;

    TwoHalfD::XYVector mouseDelta = event.mouseMove.moveDelta;//m_engine.getMouseDeltaFrame();
    float newAngle = m_gameState.playerState.playerPos.direction - (mouseDelta.x) / 200.f;
    newAngle = std::fmod(newAngle, 2 * std::numbers::pi_v<float>);
    if (newAngle < 0) newAngle += 2 * std::numbers::pi_v<float>;
    m_gameState.playerState.playerPos.direction = newAngle;
 

}

TwoHalfD::Position Game::showPosition() {
    return m_gameState.playerState.playerPos;
}
