#include "game.h"
#include <cassert>

void Game::run() {
    TwoHalfD::Level level;
    m_engine.loadLevel(level);
    while (m_engine.getState() == TwoHalfD::EngineState::running) {
        handleFrameInputs();
        m_engine.render();
    }
}

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
            default:
                break;

        }
    }   
    m_engine.clearFrameInputs();
}

void Game::handleKeyPressedEvent(const TwoHalfD::Event & event) {
    assert(event.type == TwoHalfD::Event::Type::KeyPressed);  
    std::cout << "Key pressed: " << event.key.keyCode << "\n";
}

void Game::handleKeyReleasedEvent(const TwoHalfD::Event & event) {
    assert(event.type == TwoHalfD::Event::Type::KeyReleased);
    std::cout << "Key released: " << event.key.keyCode << "\n";
}



TwoHalfD::Position Game::showPosition() {
    return m_gameState.playerState.playerPos;
}
