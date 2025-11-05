#include <SFML/Graphics.hpp>
#include <TwoHalfD/engine.h>
#include <iostream>

#include "game.h"

int main()
{
    TwoHalfD::EngineSettings engineSettings;
    TwoHalfD::Engine engine(engineSettings);

    Game game{};

    game.run();
    return 0;
}
