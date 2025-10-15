#ifndef GAME_H
#define GAME_H

#include <TwoHalfD/engine.h>

struct GameState {
    struct PlayerState {
        TwoHalfD::Position playerPos;
        struct MoveDireaction {float w, a, s, d;};

        MoveDireaction moveDir {0, 0, 0, 0};
    };

    std::array<int, 2> m_prevMousePos {0, 0};


    PlayerState playerState {};
    GameState() = default;
};



class Game
{
private:
    GameState m_gameState;
    TwoHalfD::Engine m_engine;

public:
    
    Game() : m_gameState{}, m_engine(TwoHalfD::EngineSettings{}) {};
    

    void run();
    void updateGameState();
    // Input handleing
    void handleFrameInputs();
    void handleKeyPressedEvent(const TwoHalfD::Event & event);
    void handleKeyReleasedEvent(const TwoHalfD::Event & event);
    void handleMouseMoveEvent(const TwoHalfD::Event & event); 



    
    TwoHalfD::Position showPosition();
};

#endif
