#include "TwoHalfD/input_manager.h"

#include <SFML/Window/Mouse.hpp>
#include <cmath>

TwoHalfD::InputManager::InputManager(sf::RenderWindow &window)
    : m_window(window) {}

std::span<const TwoHalfD::Event> TwoHalfD::InputManager::pollEvents(EngineState &engineState) {
    while (const std::optional event = m_window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            m_window.close();
            engineState = EngineState::ended;
        } else if (const auto *keyPressed = event->getIf<sf::Event::KeyPressed>()) {
            sf::Vector2i mouseWinPos = sf::Mouse::getPosition(m_window);
            m_inputArray[m_currentInput] = TwoHalfD::Event::KeyPressed(static_cast<int>(keyPressed->code), mouseWinPos.x, mouseWinPos.y);
            ++m_currentInput;
        } else if (const auto *keyReleased = event->getIf<sf::Event::KeyReleased>()) {
            sf::Vector2i mouseWinPos = sf::Mouse::getPosition(m_window);
            m_inputArray[m_currentInput] = TwoHalfD::Event::KeyReleased(static_cast<int>(keyReleased->code), mouseWinPos.x, mouseWinPos.y);
            ++m_currentInput;
        } else if (const auto *mouseMoved = event->getIf<sf::Event::MouseMoved>()) {
            XYVector mouseWinPos = {mouseMoved->position.x, mouseMoved->position.y};

            m_context.MouseDelta = m_context.prevMousePosition - mouseWinPos;
            m_context.prevMousePosition = m_context.currentMousePosition;
            m_context.currentMousePosition = mouseWinPos;

            if (m_warpPending) {
                m_warpPending = false;
                m_context.prevMousePosition = mouseWinPos;
                continue;
            }

            m_inputArray[m_currentInput] = TwoHalfD::Event::MouseMoved(mouseMoved->position.x, mouseMoved->position.y, m_context.MouseDelta);
            ++m_currentInput;
        }
    }
    return std::span<const TwoHalfD::Event>(m_inputArray.data(), m_currentInput);
}

void TwoHalfD::InputManager::clearFrameInputs() {
    m_currentInput = 0;
}

void TwoHalfD::InputManager::notifyWarp() {
    m_warpPending = true;
}

TwoHalfD::XYVector TwoHalfD::InputManager::getMouseDeltaFrame() const {
    return m_context.MouseDelta;
}
