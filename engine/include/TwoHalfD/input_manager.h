#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <SFML/Graphics.hpp>
#include <array>
#include <span>

#include "TwoHalfD/engine_types.h"

namespace TwoHalfD {

class InputManager {
  public:
    InputManager(sf::RenderWindow &window, const EngineSettings &settings);

    std::span<const Event> pollEvents(EngineState &engineState);
    void clearFrameInputs();
    void notifyWarp();
    XYVector getMouseDeltaFrame() const;

  private:
    sf::RenderWindow &m_window;
    const EngineSettings &m_settings;

    EngineContext m_context;
    std::array<Event, 512> m_inputArray{};
    int m_currentInput{0};
    bool m_warpPending{false};
};

} // namespace TwoHalfD

#endif
