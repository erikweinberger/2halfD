#ifndef GAME_H
#define GAME_H

#include <TwoHalfD/engine.h>

struct GameState {
    TwoHalfD::Position playerPos;
    GameState() = default;
};



class Game
{
private:
    GameState m_gameState;

public:
    
    Game() : m_gameState() {};

    TwoHalfD::Position showPosition();
    void handleFrameInputs();
};

#endif
