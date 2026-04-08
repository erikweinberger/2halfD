#include "TwoHalfD/input_manager.h"

#include <SFML/Window/Mouse.hpp>
#include <cmath>

TwoHalfD::InputManager::InputManager(sf::RenderWindow &window)
    : m_window(window) {}

std::span<const TwoHalfD::Event> TwoHalfD::InputManager::pollEvents(EngineState &engineState) {
    sf::Event event;
    while (m_window.pollEvent(event)) {
        switch (event.type) {
        case sf::Event::Closed: {
            m_window.close();
            engineState = EngineState::ended;
            break;
        }
        case sf::Event::KeyPressed: {
            sf::Vector2i mouseWinPos = sf::Mouse::getPosition(m_window);
            m_inputArray[m_currentInput] = TwoHalfD::Event::KeyPressed(event.key.code, mouseWinPos.x, mouseWinPos.y);
            ++m_currentInput;
            break;
        }
        case sf::Event::KeyReleased: {
            sf::Vector2i mouseWinPos = sf::Mouse::getPosition(m_window);
            m_inputArray[m_currentInput] = TwoHalfD::Event::KeyReleased(event.key.code, mouseWinPos.x, mouseWinPos.y);
            ++m_currentInput;
            break;
        }
        case sf::Event::MouseMoved: {
            XYVector mouseWinPos = {event.mouseMove.x, event.mouseMove.y};

            m_context.MouseDelta = m_context.prevMousePosition - mouseWinPos;
            m_context.prevMousePosition = m_context.currentMousePosition;
            m_context.currentMousePosition = mouseWinPos;

            if (m_warpPending) {
                m_warpPending = false;
                m_context.prevMousePosition = mouseWinPos;
                break;
            }

            m_inputArray[m_currentInput] = TwoHalfD::Event::MouseMoved(event.mouseMove.x, event.mouseMove.y, m_context.MouseDelta);
            ++m_currentInput;
            break;
        }
        default:
            break;
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
