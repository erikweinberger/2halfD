#include <iostream>
#include <SFML/Graphics.hpp>
#include <TwoHalfD/engine.h>

#include "game.h"

int main () {
    TwoHalfD::EngineSettings engineSettings;
    TwoHalfD::Engine engine(engineSettings);
    
    Game game {};
    TwoHalfD::Position position = game.showPosition();
    std::cout << "Position: " << position.u.pos.x << " , " << position.u.pos.y << std::endl;
    std::cout << "Position_sf: " << position.u.posf.x << " , " << position.u.posf.y << std::endl;

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
