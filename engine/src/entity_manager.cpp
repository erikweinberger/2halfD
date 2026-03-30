#include "TwoHalfD/types/entity_types.h"
#include <TwoHalfD/entity_manager.h>

TwoHalfD::EntityManager::EntityManager() = default;
TwoHalfD::EntityManager::~EntityManager() = default;

void TwoHalfD::EntityManager::addEntity(TwoHalfD::SpriteEntity entity) {
    m_entities[entity.id] = std::move(entity);
}

void TwoHalfD::EntityManager::removeEntity(int id) {
    m_entities.erase(id);
}

std::optional<TwoHalfD::SpriteEntity> TwoHalfD::EntityManager::getEntity(int id) const {
    auto it = m_entities.find(id);
    if (it == m_entities.end()) return std::nullopt;
    return it->second;
}

const std::unordered_map<int, TwoHalfD::SpriteEntity> &TwoHalfD::EntityManager::getAllEntities() const {
    return m_entities;
}

void TwoHalfD::EntityManager::setHeightStart(int entityId, float heightStart) {
    auto it = m_entities.find(entityId);
    if (it != m_entities.end()) {
        it->second.heightStart = heightStart;
    }
}

void TwoHalfD::EntityManager::walkTo(int entityId, const TwoHalfD::Path &path) {
    if (path.empty()) return;
    auto it = m_entities.find(entityId);
    if (it == m_entities.end()) return;
    it->second.currentUpdate = TwoHalfD::WalkToUpdate{path.back(), path, 1};
}

std::vector<std::pair<int, TwoHalfD::XYVectorf>> TwoHalfD::EntityManager::update([[maybe_unused]] float deltaTime) {
    std::vector<std::pair<int, TwoHalfD::XYVectorf>> movedEntities;

    for (auto &[id, entity] : m_entities) {
        if (!entity.currentUpdate) continue;

        TwoHalfD::XYVectorf prevPos = entity.pos.pos;

        std::visit(
            [&](auto &update) {
                using T = std::decay_t<decltype(update)>;
                if constexpr (std::is_same_v<T, TwoHalfD::WalkToUpdate>) {
                    _tickWalkTo(entity, update);
                }
            },
            *entity.currentUpdate);

        if (entity.pos.pos.x != prevPos.x || entity.pos.pos.y != prevPos.y) {
            movedEntities.push_back({id, entity.pos.pos});
        }
    }

    return movedEntities;
}

void TwoHalfD::EntityManager::_tickWalkTo(TwoHalfD::SpriteEntity &entity, TwoHalfD::WalkToUpdate &update) {
    if (update.nextPathIndex >= update.path.size()) {
        entity.currentUpdate = std::nullopt;
        return;
    }

    const auto &targetPos = update.path[update.nextPathIndex];
    auto direction = (targetPos - entity.pos.pos).normalized();
    entity.pos.pos = entity.pos.pos + direction * entity.speed;

    if ((entity.pos.pos - targetPos).length() < entity.speed + 1.f) {
        update.nextPathIndex++;
        if (update.nextPathIndex >= update.path.size()) {
            entity.currentUpdate = std::nullopt;
        }
    }
}
