#include <iostream>
#include <SFML/Graphics.hpp>

#include <TwoHalfD/engine.h>

int main () {
    TwoHalfD::EngineSettings engineSettings;
    TwoHalfD::Engine engine(engineSettings);
    
    TwoHalfD::Level level;
    engine.loadLevel(level);
    while (engine.getState() == TwoHalfD::EngineState::running) {
        std::span<const TwoHalfD::Event> inputs = engine.getFrameInputs();
        engine.render();
    }

    std::cout << "Hello World" << "\n";
    sf::Vector2f test { 10.f, 10.f };
    return 0;
}
