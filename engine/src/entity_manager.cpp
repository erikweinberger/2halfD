#include "TwoHalfD/bsp/bsp_manager.h"
#include "TwoHalfD/engine_types.h"
#include "TwoHalfD/types/animation_types.h"
#include "TwoHalfD/types/bsp_types.h"
#include "TwoHalfD/types/entity_types.h"
#include <TwoHalfD/entity_manager.h>
#include <algorithm>

TwoHalfD::EntityManager::EntityManager() = default;
TwoHalfD::EntityManager::~EntityManager() = default;

void TwoHalfD::EntityManager::addEntity(TwoHalfD::SpriteEntity entity) {
    m_entities[entity.id] = std::move(entity);
}

void TwoHalfD::EntityManager::removeEntity(int id) {
    m_entities.erase(id);
}

std::optional<const TwoHalfD::SpriteEntity> TwoHalfD::EntityManager::getEntity(int id) const {
    auto it = m_entities.find(id);
    if (it == m_entities.end()) return std::nullopt;
    return it->second;
}

const TwoHalfD::SpriteEntity *TwoHalfD::EntityManager::getEntityPtr(int id) const {
    auto it = m_entities.find(id);
    if (it == m_entities.end()) return nullptr;
    return &it->second;
}

const std::unordered_map<int, TwoHalfD::SpriteEntity> &TwoHalfD::EntityManager::getAllEntities() const {
    return m_entities;
}

void TwoHalfD::EntityManager::walkTo(int entityId, const TwoHalfD::Path &path) {
    if (path.empty()) return;
    auto it = m_entities.find(entityId);
    if (it == m_entities.end()) return;
    it->second.currentUpdate = TwoHalfD::WalkToUpdate{path.back(), path, 1};
}

void TwoHalfD::EntityManager::setHeightStart(int entityId, float heightStart) {
    auto it = m_entities.find(entityId);
    if (it != m_entities.end()) {
        it->second.heightStart = heightStart;
    }
}

void TwoHalfD::EntityManager::setFloorHeight(int entityId, float floorHeight) {
    auto it = m_entities.find(entityId);
    if (it != m_entities.end()) {
        it->second.floorHeight = floorHeight;
    }
}

void TwoHalfD::EntityManager::setEntityPerimeterRegion(int entityId, int perimeterIndex, const BSPNode *bspRegion) {
    m_entities[entityId].perimeterPoints[perimeterIndex].bspRegion = bspRegion;
}

std::vector<std::pair<int, TwoHalfD::XYVectorf>> TwoHalfD::EntityManager::update(float deltaTime, const EngineSettings &engineSettings,
                                                                                 const BSPManager &bsp, const CameraObject &cameraObject) {
    std::vector<std::pair<int, TwoHalfD::XYVectorf>> movedEntities;

    for (auto &[id, entity] : m_entities) {

        TwoHalfD::XYVectorf prevPos = entity.pos.pos;
        bool isFalling = false;

        float maxPerimeterFloor = entity.floorHeight;
        for (const auto &p : entity.perimeterPoints) {
            float h = p.height();
            if (h - entity.heightStart < engineSettings.heightClipping) maxPerimeterFloor = std::max(maxPerimeterFloor, h);
        }

        if (maxPerimeterFloor < entity.heightStart) {
            isFalling = true;
            float gravity = entity.gravityOverride.value_or(engineSettings.gravity);
            float maxFallSpeed = entity.maxFallSpeedOverride.value_or(engineSettings.maxFallSpeed);
            entity.velocity.z -= gravity;
            if (-entity.velocity.z > maxFallSpeed) {
                entity.velocity.z = -maxFallSpeed;
            }
            entity.heightStart += entity.velocity.z;
            if (entity.heightStart <= maxPerimeterFloor) {
                entity.heightStart = maxPerimeterFloor;
                entity.velocity.z = 0.f;
            }
        } else if (maxPerimeterFloor > entity.heightStart) {
            entity.heightStart = maxPerimeterFloor;
            entity.velocity.z = 0.f;
        }
        if (!entity.currentUpdate) continue;
        if (!isFalling || entity.canMoveWhileFallingOverride.value_or(engineSettings.canMoveWhileFalling)) {
            std::visit(
                [&](auto &update) {
                    using T = std::decay_t<decltype(update)>;
                    if constexpr (std::is_same_v<T, TwoHalfD::WalkToUpdate>) {
                        _tickWalkTo(entity, update, engineSettings, bsp, cameraObject);
                    }
                },
                *entity.currentUpdate);
        }

        if (entity.currentAnimation && _tickAnimation(*entity.currentAnimation, deltaTime)) entity.currentAnimation = std::nullopt;

        for (int i = static_cast<int>(entity.overlays.count) - 1; i >= 0; --i) {
            auto &overlay = entity.overlays.overlays[i];
            if (overlay.active && _tickAnimation(overlay.animState, deltaTime)) {
                entity.overlays.remove(overlay.overlayId);
            }
        }

        if (entity.pos.pos.x != prevPos.x || entity.pos.pos.y != prevPos.y) {
            movedEntities.push_back({id, entity.pos.pos});
        }
    }

    m_expiredEffectIds.clear();
    for (auto &[id, effect] : m_effects) {
        if (_tickAnimation(effect.animState, deltaTime)) {
            m_expiredEffectIds.push_back(id);
        }
    }

    return movedEntities;
}

void TwoHalfD::EntityManager::setAnimationTemplates(const std::unordered_map<int, TwoHalfD::AnimationTemplate> &templates) {
    m_animationTemplates = &templates;
}

const std::unordered_map<int, TwoHalfD::AnimationTemplate> *TwoHalfD::EntityManager::getAnimationTemplates() const {
    return m_animationTemplates;
}

void TwoHalfD::EntityManager::setAnimation(int entityId, int templateId, bool loop) {
    auto it = m_entities.find(entityId);
    if (it == m_entities.end()) return;
    it->second.currentAnimation = TwoHalfD::AnimationState{templateId, 0, 0.f, loop};
}

void TwoHalfD::EntityManager::clearAnimation(int entityId) {
    auto it = m_entities.find(entityId);
    if (it == m_entities.end()) return;
    it->second.currentAnimation = std::nullopt;
}

int TwoHalfD::EntityManager::addOverlay(int entityId, int templateId, float x, float y, float width, float height, int zOrder, bool loop,
                                        float textureScaleX, float textureScaleY) {
    auto it = m_entities.find(entityId);
    if (it == m_entities.end()) return -1;
    return it->second.overlays.add(templateId, x, y, width, height, zOrder, loop, textureScaleX, textureScaleY);
}

void TwoHalfD::EntityManager::removeOverlay(int entityId, int overlayId) {
    auto it = m_entities.find(entityId);
    if (it == m_entities.end()) return;
    it->second.overlays.remove(overlayId);
}

void TwoHalfD::EntityManager::clearOverlays(int entityId) {
    auto it = m_entities.find(entityId);
    if (it == m_entities.end()) return;
    it->second.overlays.clear();
}

int TwoHalfD::EntityManager::spawnEffect(TwoHalfD::XYVectorf pos, int templateId, float height, float width, float scaleX, float scaleY,
                                         float heightStart) {
    int id = m_nextEffectId++;
    m_effects[id] =
        TwoHalfD::AnimationEffect{id, pos, heightStart, height, width, scaleX, scaleY, TwoHalfD::AnimationState{templateId, 0, 0.f, false}};
    return id;
}

void TwoHalfD::EntityManager::removeEffect(int effectId) {
    m_effects.erase(effectId);
}

void TwoHalfD::EntityManager::setEffectHeightStart(int effectId, float heightStart) {
    auto it = m_effects.find(effectId);
    if (it != m_effects.end()) {
        it->second.heightStart = heightStart;
    }
}

const std::unordered_map<int, TwoHalfD::AnimationEffect> &TwoHalfD::EntityManager::getAllEffects() const {
    return m_effects;
}

const std::vector<int> &TwoHalfD::EntityManager::getExpiredEffectIds() const {
    return m_expiredEffectIds;
}

void TwoHalfD::EntityManager::eraseExpiredEffects() {
    for (int id : m_expiredEffectIds) {
        m_effects.erase(id);
    }
    m_expiredEffectIds.clear();
}

bool TwoHalfD::EntityManager::_tickAnimation(TwoHalfD::AnimationState &state, float deltaTime) {
    if (!m_animationTemplates) return false;
    auto it = m_animationTemplates->find(state.templateId);
    if (it == m_animationTemplates->end()) return false;

    const auto &tmpl = it->second;
    if (tmpl.frames.empty()) return false;

    state.elapsedTime += deltaTime;
    const auto &currentFrame = tmpl.frames[state.frameIndex];

    while (state.elapsedTime >= currentFrame.duration) {
        state.elapsedTime -= currentFrame.duration;
        state.frameIndex++;

        if (state.frameIndex >= static_cast<int>(tmpl.frames.size())) {
            if (state.loop) {
                state.frameIndex = 0;
            } else {
                return true; // finished
            }
        }
    }
    return false;
}

void TwoHalfD::EntityManager::_tickWalkTo(TwoHalfD::SpriteEntity &entity, TwoHalfD::WalkToUpdate &update, const EngineSettings &engineSettings,
                                          const BSPManager &bsp, const CameraObject &cameraObject) {
    if (update.nextPathIndex >= update.path.size()) {
        entity.currentUpdate = std::nullopt;
        return;
    }

    const auto &targetPos = update.path[update.nextPathIndex];
    auto direction = (targetPos - entity.pos.pos).normalized();
    entity.pos.pos = entity.pos.pos + direction * entity.speed;

    std::vector<const BSPNode *> bspRegions;
    bspRegions.reserve(entity.perimeterPoints.size());
    for (const auto &boundingPoint : entity.perimeterPoints) {
        if (std::find(bspRegions.begin(), bspRegions.end(), boundingPoint.bspRegion) == bspRegions.end()) {
            bspRegions.push_back(boundingPoint.bspRegion);
        }
    }

    std::vector<const Segment *> boundingSegments;
    std::vector<const SpriteEntity *> otherSprites;
    for (const auto bspRegion : bspRegions) {
        if (bspRegion == nullptr) continue;
        for (const auto segmentId : bspRegion->boundingSegmentIds) {
            const auto segment = &bsp.getSegment(segmentId);
            if (std::find(boundingSegments.begin(), boundingSegments.end(), segment) == boundingSegments.end()) {
                boundingSegments.push_back(segment);
            }
        }
        for (const auto spriteId : bspRegion->spriteIds) {
            if (spriteId == entity.id) continue;
            auto sprite = this->getEntityPtr(spriteId);
            if (sprite) {
                otherSprites.push_back(sprite);
            }
        }
    }

    for (const auto segment : boundingSegments) {
        if (segment->isWall() && entity.heightStart >= segment->wall->wallHeightStart + segment->wall->height) continue;
        if (segment->isFloorBoundary() && std::abs(segment->floorSection->height - entity.heightStart) < engineSettings.heightClipping) continue;
        auto segVec = segment->v2 - segment->v1;
        float t = dot(entity.pos.pos - segment->v1, segVec) / segVec.lengthSquared();
        t = std::clamp(t, 0.f, 1.f);
        auto closestPoint = segment->v1 + t * segVec;
        auto pushVec = entity.pos.pos - closestPoint;
        float dist = pushVec.length();
        if (dist < entity.radius && dist > 0.f) {
            float penetration = entity.radius - dist;
            entity.pos.pos += pushVec.normalized() * penetration;
        }
    }

    for (auto &sprite : otherSprites) {
        auto otherSpriteToEntityVector = (entity.pos.pos - sprite->pos.pos);
        float cameraToSpriteDist = otherSpriteToEntityVector.length();
        if (cameraToSpriteDist <= (sprite->radius + entity.radius)) {
            entity.pos.pos += otherSpriteToEntityVector.normalized() * ((sprite->radius + entity.radius + 1.f) - cameraToSpriteDist);
        }
    }

    auto cameraToSpriteVector = (entity.pos.pos - cameraObject.cameraPos.pos);
    float cameraToSpriteDist = cameraToSpriteVector.length();
    if (cameraToSpriteDist <= (cameraObject.cameraRadius + entity.radius)) {
        entity.pos.pos += cameraToSpriteVector.normalized() * ((cameraObject.cameraRadius + entity.radius + 1.f) - cameraToSpriteDist);
    }

    if ((entity.pos.pos - targetPos).length() < entity.speed + 1.f) {
        update.nextPathIndex++;
        if (update.nextPathIndex >= update.path.size()) {
            entity.currentUpdate = std::nullopt;
        }
    }
}
