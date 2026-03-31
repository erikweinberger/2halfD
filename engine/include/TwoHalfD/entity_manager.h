#ifndef ENTITY_MANAGER_H
#define ENTITY_MANAGER_H

#include "TwoHalfD/types/animation_types.h"
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

    void setAnimation(int entityId, int templateId, bool loop = false);
    void clearAnimation(int entityId);
    int addOverlay(int entityId, int templateId, float x = 0.5f, float y = 0.5f, float scale = 1.f, int zOrder = 0, bool loop = false);
    void removeOverlay(int entityId, int overlayId);
    void clearOverlays(int entityId);

    std::vector<std::pair<int, TwoHalfD::XYVectorf>> update(float deltaTime);

    void setAnimationTemplates(const std::unordered_map<int, TwoHalfD::AnimationTemplate> &templates);
    const std::unordered_map<int, TwoHalfD::AnimationTemplate> *getAnimationTemplates() const;

  private:
    std::unordered_map<int, TwoHalfD::SpriteEntity> m_entities;
    const std::unordered_map<int, TwoHalfD::AnimationTemplate> *m_animationTemplates = nullptr;

    void _tickWalkTo(TwoHalfD::SpriteEntity &entity, TwoHalfD::WalkToUpdate &update);
    bool _tickAnimation(TwoHalfD::AnimationState &state, float deltaTime);
};
} // namespace TwoHalfD

#endif
