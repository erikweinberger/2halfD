#include "TwoHalfD/types/bsp_types.h"
#include "TwoHalfD/types/entity_types.h"
#include "TwoHalfD/types/math_types.h"
#include "TwoHalfD/utils/math_util.h"
#include <TwoHalfD/engine.h>

#include <SFML/Window/Mouse.hpp>
#include <cmath>
#include <objc/objc.h>
#include <span>
#include <unordered_map>
#include <variant>

void TwoHalfD::Engine::loadLevel(std::string levelFilePath) {
    this->m_engineState = EngineState::fpsState;
    m_window.setMouseCursorVisible(false);

    TwoHalfD::Level level = m_levelMaker.parseLevelFile(levelFilePath);

    m_textures = std::move(level.textures);
    m_defaultFloorHeight = level.defaultFloorHeight;
    m_defaultFloorTextureId = level.defaultFloorTextureId;
    m_defaultFloorStart = level.defaultFloorStart;

    m_animationTemplates = std::move(level.animationTemplates);
    m_entityManager.setAnimationTemplates(m_animationTemplates);

    for (auto &sprite : level.sprites) {
        m_entityManager.addEntity(std::move(sprite));
    }

    m_bspManager.init(std::move(level.walls), std::move(level.floorSections), m_defaultFloorHeight, m_defaultFloorTextureId, level.seed);
    m_bspManager.buildBSPTree();
    m_bspManager.buildGraph();

    auto heightStarts = m_bspManager.insertSprites(m_entityManager.getAllEntities());
    for (const auto &[entityId, heightStart] : heightStarts) {
        m_entityManager.setHeightStart(entityId, heightStart);
        m_entityManager.setFloorHeight(entityId, heightStart);
    }

    m_renderer.setData(&m_textures, &m_entityManager, m_defaultFloorHeight, m_defaultFloorTextureId, m_defaultFloorStart);
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

        if (m_window.hasFocus() && (mousePosition.x < 0 || mousePosition.x >= (int)size.x || mousePosition.y < 0 || mousePosition.y >= (int)size.y)) {
            sf::Mouse::setPosition({middleScreen.x, middleScreen.y}, m_window);
            m_inputManager.notifyWarp();
        }
    }

    float deltaTime = static_cast<float>(m_engineClocks.getGameDeltaTime());
    auto movedEntities = m_entityManager.update(deltaTime, m_engineSettings);
    for (const auto &[entityId, newPos] : movedEntities) {
        moveSprite(entityId, newPos);
    }

    for (int effectId : m_entityManager.getExpiredEffectIds()) {
        m_bspManager.removeEffect(effectId);
    }
    m_entityManager.eraseExpiredEffects();

    auto convexSection = m_bspManager.findConvexSection(m_cameraObject.cameraPos.pos);
    m_cameraObject.cameraFloorHeight =
        convexSection != nullptr && convexSection->floorSection != nullptr ? convexSection->floorSection->height : m_defaultFloorHeight;
    m_cameraObject.nodeId = convexSection != nullptr ? convexSection->leafNodeId : -1;

    for (size_t i = 0; i < m_cameraObject.perimeterPoints.size(); ++i) {
        auto *section = m_bspManager.findConvexSection(m_cameraObject.cameraPos.pos + m_cameraObject.perimeterPoints[i].offset);
        m_cameraObject.perimeterPoints[i].floorHeight = (section && section->floorSection) ? section->floorSection->height : m_defaultFloorHeight;
        m_cameraObject.perimeterPoints[i].nodeId = (section) ? section->leafNodeId : -1;
    }

    float maxPerimeterFloor =
        std::max_element(m_cameraObject.perimeterPoints.begin(), m_cameraObject.perimeterPoints.end(), [](const auto &p1, const auto &p2) {
            return p1.floorHeight < p2.floorHeight;
        })->floorHeight;

    if (maxPerimeterFloor < m_cameraObject.cameraHeightStart) {
        float gravity = m_cameraObject.gravityOverride.value_or(m_engineSettings.gravity);
        float maxFallSpeed = m_cameraObject.maxFallSpeedOverride.value_or(m_engineSettings.maxFallSpeed);
        m_cameraObject.velocity.z -= gravity;
        if (-m_cameraObject.velocity.z > maxFallSpeed) {
            m_cameraObject.velocity.z = -maxFallSpeed;
        }
        m_cameraObject.cameraHeightStart += m_cameraObject.velocity.z;
        if (m_cameraObject.cameraHeightStart <= maxPerimeterFloor) {
            m_cameraObject.cameraHeightStart = maxPerimeterFloor;
            m_cameraObject.velocity.z = 0.f;
        }
    } else {
        m_cameraObject.cameraHeightStart = maxPerimeterFloor;
        m_cameraObject.velocity.z = 0.f;
    }
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
    float maxFloor = m_cameraObject.cameraFloorHeight;
    for (const auto &point : m_cameraObject.perimeterPoints) {
        maxFloor = std::max(maxFloor, point.floorHeight);
    }
    bool isFalling = maxFloor < m_cameraObject.cameraHeightStart;
    if (isFalling && !m_cameraObject.canMoveWhileFallingOverride.value_or(m_engineSettings.canMoveWhileFalling)) {
        return m_cameraObject.cameraPos;
    }

    TwoHalfD::Position prevPos = m_cameraObject.cameraPos;
    m_cameraObject.cameraPos += posUpdate;
    TwoHalfD::XYVectorf moveVec{posUpdate.pos.x, posUpdate.pos.y};

    if (m_engineSettings.cameraCollision) {
        std::unordered_set<int> nodeIds;
        nodeIds.reserve(m_cameraObject.perimeterPoints.size());
        for (const auto boundingPoint : m_cameraObject.perimeterPoints) {
            nodeIds.insert(boundingPoint.nodeId);
        }
        std::unordered_set<const Segment *> boundingSegments;
        for (const auto nodeId : nodeIds) {
            auto node = m_bspManager.getLeafNode(nodeId);
            for (const auto segmentId : node->boundingSegmentIds) {
                boundingSegments.insert(&m_bspManager.getSegment(segmentId));
            }
        }

        for (const auto segment : boundingSegments) {
            if (segment->isWall() && m_cameraObject.cameraHeightStart >= segment->wall->wallHeightStart + segment->wall->height) continue;
            if (segment->isFloorBoundary() && segment->floorSection->height >= m_engineSettings.heightClipping) continue;

            auto segVec = segment->v2 - segment->v1;
            float t = dot(m_cameraObject.cameraPos.pos - segment->v1, segVec) / dot(segVec, segVec);
            t = std::clamp(t, 0.f, 1.f);
            auto closestPoint = segment->v1 + t * segVec;
            auto pushVec = m_cameraObject.cameraPos.pos - closestPoint;
            float dist = pushVec.length();
            if (dist < m_cameraObject.cameraRadius && dist > 0.f) {
                float penetration = m_cameraObject.cameraRadius - dist;
                m_cameraObject.cameraPos.pos = m_cameraObject.cameraPos.pos + pushVec.normalized() * penetration;
            }
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

const std::unordered_map<int, TwoHalfD::SpriteEntity> &TwoHalfD::Engine::getAllSpriteEntities() {
    return m_entityManager.getAllEntities();
}

const std::vector<TwoHalfD::Wall> &TwoHalfD::Engine::getAllWalls() {
    return m_bspManager.getWalls();
}

void TwoHalfD::Engine::render() {
    m_renderer.render(m_cameraObject, m_bspManager);
}

void TwoHalfD::Engine::walkTo(const int entityId, const TwoHalfD::XYVectorf targetPos, float maxHeightDiff, float maxStepDown, float maxDistance) {
    auto entity = m_entityManager.getEntity(entityId);
    if (!entity) return;
    auto path = getPathfindingPoints(entity->pos.pos, targetPos, entity->radius, maxHeightDiff, maxStepDown, maxDistance);
    m_entityManager.walkTo(entityId, path);
}

void TwoHalfD::Engine::setAnimation(int entityId, int templateId, bool loop) {
    m_entityManager.setAnimation(entityId, templateId, loop);
}
void TwoHalfD::Engine::clearAnimation(int entityId) {
    m_entityManager.clearAnimation(entityId);
}
int TwoHalfD::Engine::addOverlay(int entityId, int templateId, float x, float y, float width, float height, int zOrder, bool loop,
                                 float textureScaleX, float textureScaleY) {
    return m_entityManager.addOverlay(entityId, templateId, x, y, width, height, zOrder, loop, textureScaleX, textureScaleY);
}
void TwoHalfD::Engine::removeOverlay(int entityId, int overlayId) {
    m_entityManager.removeOverlay(entityId, overlayId);
}
void TwoHalfD::Engine::clearOverlays(int entityId) {
    m_entityManager.clearOverlays(entityId);
}

int TwoHalfD::Engine::spawnEffect(TwoHalfD::XYVectorf pos, int templateId, float height, float width, float scaleX, float scaleY, float heightStart) {
    if (heightStart < 0.f) {
        auto *section = m_bspManager.findConvexSection(pos);
        heightStart = (section && section->floorSection) ? section->floorSection->height : m_defaultFloorHeight;
    }
    int id = m_entityManager.spawnEffect(pos, templateId, height, width, scaleX, scaleY, heightStart);
    m_bspManager.insertEffect(id, pos);
    return id;
}

void TwoHalfD::Engine::removeEffect(int effectId) {
    m_bspManager.removeEffect(effectId);
    m_entityManager.removeEffect(effectId);
}

std::vector<TwoHalfD::XYVectorf> TwoHalfD::Engine::getPathfindingPoints(TwoHalfD::XYVectorf start, TwoHalfD::XYVectorf end, float entityWidth,
                                                                        float maxHeightDiff, float maxStepDown, float maxDistance) {
    return m_bspManager.findPath(start, end, entityWidth, maxHeightDiff, maxStepDown, maxDistance);
}

void TwoHalfD::Engine::addColourOverlay(int id, const TwoHalfD::Polygon &vertices, float height, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    m_bspManager.insertColourOverlay(id, vertices, height, r, g, b, a);
}

void TwoHalfD::Engine::updateColourOverlay(int id, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    m_bspManager.updateColourOverlay(id, r, g, b, a);
}

void TwoHalfD::Engine::removeColourOverlay(int id) {
    m_bspManager.removeColourOverlay(id);
}

void TwoHalfD::Engine::moveSprite(int entityId, TwoHalfD::XYVectorf newPos) {
    auto entity = m_entityManager.getEntity(entityId);
    m_bspManager.moveSprite(entityId, newPos);

    for (const auto &perimeterPoint : entity->perimeterPoints) {
        auto perimeterSection = m_bspManager.findConvexSection(newPos + perimeterPoint.offset);
        float floorHeight = (perimeterSection && perimeterSection->floorSection) ? perimeterSection->floorSection->height : m_defaultFloorHeight;
        m_entityManager.setPerimeterFloorHeight(entityId, &perimeterPoint - &entity->perimeterPoints[0], floorHeight);
    }
}
