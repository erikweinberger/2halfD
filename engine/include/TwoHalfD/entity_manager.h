#ifndef ENTITY_MANAGER_H
#define ENTITY_MANAGER_H

#include "TwoHalfD/types/entity_types.h"
#include <optional>
#include <unordered_map>
#include <vector>

namespace TwoHalfD {

class EntityManager {
  public:
    EntityManager();
    ~EntityManager();

    void addEntity(TwoHalfD::SpriteEntity entity);
    void removeEntity(int id);
    std::optional<TwoHalfD::SpriteEntity> getEntity(int id) const;
    const std::unordered_map<int, TwoHalfD::SpriteEntity> &getAllEntities() const;

    void walkTo(int entityId, const TwoHalfD::Path &path);
    void setHeightStart(int entityId, float heightStart);

    std::vector<std::pair<int, TwoHalfD::XYVectorf>> update(float deltaTime);

  private:
    std::unordered_map<int, TwoHalfD::SpriteEntity> m_entities;

    void _tickWalkTo(TwoHalfD::SpriteEntity &entity, TwoHalfD::WalkToUpdate &update);
};
} // namespace TwoHalfD

#endif
