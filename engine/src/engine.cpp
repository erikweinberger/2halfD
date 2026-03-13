#include "TwoHalfD/utils/math_util.h"
#include <TwoHalfD/engine.h>

#include <SFML/Window/Mouse.hpp>
#include <cmath>
#include <span>

void TwoHalfD::Engine::loadLevel(std::string levelFilePath) {
    this->m_engineState = EngineState::fpsState;
    m_window.setMouseCursorVisible(false);
    m_level = m_levelMaker.parseLevelFile(levelFilePath);
    m_bspManager.setLevel(&m_level);
    m_bspManager.buildBSPTree();
    m_bspManager.buildGraph();
}

// Game Inputs
std::span<const TwoHalfD::Event> TwoHalfD::Engine::getFrameInputs() {
    auto events = m_inputManager.pollEvents(m_engineState);
    backgroundFrameUpdates();
    return events;
}

void TwoHalfD::Engine::clearFrameInputs() {
    m_inputManager.clearFrameInputs();
}

void TwoHalfD::Engine::backgroundFrameUpdates() {
    if (m_engineState == TwoHalfD::EngineState::fpsState) {
        auto size = m_window.getSize();
        const XYVector middleScreen = {(int)size.x / 2, (int)size.y / 2};
        sf::Vector2i mousePosition = sf::Mouse::getPosition(m_window);

        if (m_window.hasFocus() && (m_engineSettings.windowDim.x - mousePosition.x < 0 || mousePosition.x < 0 ||
                                    m_engineSettings.windowDim.y - mousePosition.y < 0 || mousePosition.y < 0)) {
            sf::Mouse::setPosition({middleScreen.x, middleScreen.y}, m_window);
        }
    }

    auto convexSection = m_bspManager.findConvexSection(m_cameraObject.cameraPos.pos);
    m_cameraObject.cameraHeightStart =
        convexSection != nullptr && convexSection->floorSection != nullptr ? convexSection->floorSection->height : m_level.defaultFloorStart.y;
}

bool TwoHalfD::Engine::gameDeltaTimePassed() {
    return m_engineClocks.gameTimeDeltaPassed();
}

TwoHalfD::Position TwoHalfD::Engine::getCameraPosition() {
    return m_cameraObject.cameraPos;
}

void TwoHalfD::Engine::setCameraPosition(const TwoHalfD::Position &newPos) {
    m_cameraObject.cameraPos = newPos;
}

TwoHalfD::Position TwoHalfD::Engine::updateCameraPosition(const TwoHalfD::Position &posUpdate) {
    TwoHalfD::Position prevPos = m_cameraObject.cameraPos;
    m_cameraObject.cameraPos += posUpdate;
    TwoHalfD::XYVectorf moveVec{posUpdate.pos.x, posUpdate.pos.y};
    TwoHalfD::XYVectorf n_moveVec{moveVec.normalized()};

    float moveMagnitude = 0;
    if (m_engineSettings.cameraCollision) {
        TwoHalfD::XYVectorf oldPos = prevPos.pos;
        auto segmentSpans = m_bspManager.findCollisions(m_cameraObject.cameraPos.pos, m_cameraObject.cameraRadius, m_cameraObject.cameraHeightStart);
        if (segmentSpans.size() > 0) {
            for (const auto &[start, end] : segmentSpans) {
                const TwoHalfD::XYVectorf segmentVec = {end.x - start.x, end.y - start.y};
                TwoHalfD::XYVectorf n_segmentVec{segmentVec.normalized()};

                const float perpPointDistToStart = TwoHalfD::dot(m_cameraObject.cameraPos.pos - start, n_segmentVec);
                TwoHalfD::XYVectorf perpP{start + n_segmentVec * perpPointDistToStart};

                TwoHalfD::XYVectorf perpVec{m_cameraObject.cameraPos.pos - perpP};
                TwoHalfD::XYVectorf n_perpVec{perpVec.normalized()};

                TwoHalfD::XYVectorf oldPerpVec{oldPos - perpP};
                float perpVecLen = perpVec.length();

                float penetrationDepth;
                if (dot(oldPerpVec, n_perpVec) < 0 && oldPerpVec.length() > perpVecLen) {
                    penetrationDepth = m_cameraObject.cameraRadius + perpVecLen;
                } else {
                    penetrationDepth = m_cameraObject.cameraRadius - perpVecLen;
                }
                float moveRatio = std::abs(dot(n_perpVec, n_moveVec));
                if (std::abs(moveRatio) < 0.01f) continue;

                moveMagnitude = std::min(std::max(moveMagnitude, penetrationDepth / moveRatio), moveVec.length());
            }

            TwoHalfD::Position newPos{m_cameraObject.cameraPos.pos - moveMagnitude * n_moveVec, m_cameraObject.cameraPos.direction};
            setCameraPosition(newPos);
            return m_cameraObject.cameraPos;
        }
    }

    return m_cameraObject.cameraPos;
}

void TwoHalfD::Engine::setState(TwoHalfD::EngineState newState) {
    m_engineState = newState;
}

TwoHalfD::EngineState TwoHalfD::Engine::getState() {
    return this->m_engineState;
}

std::vector<TwoHalfD::SpriteEntity> &TwoHalfD::Engine::getAllSpriteEntities() {
    return m_level.sprites;
}
std::vector<TwoHalfD::Wall> &TwoHalfD::Engine::getAllWalls() {
    return m_level.walls;
}

void TwoHalfD::Engine::render() {
    m_renderer.render(m_cameraObject, m_level, m_bspManager);
}
