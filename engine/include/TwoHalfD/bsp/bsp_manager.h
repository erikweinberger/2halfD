#ifndef BSP_MANAGER_H
#define BSP_MANAGER_H

#include "TwoHalfD/engine_types.h"
#include <cstddef>
#include <memory>
#include <queue>
#include <vector>

namespace TwoHalfD {

struct OptimalCostPartitioning {
    int splitCount;
    int numFront;
    int numBack;
};

class BSPManager {
  public:
    BSPManager() : m_level(nullptr) {}
    BSPManager(const TwoHalfD::Level *level) : m_level(level) {}

    void buildBSPTree();
    void insertSprites(const std::vector<SpriteEntity> &sprites);

    // Traverse logic
    std::vector<TwoHalfD::DrawCommand> update(TwoHalfD::Position &cameraPos);
    void traverse(TwoHalfD::BSPNode *node, std::vector<TwoHalfD::DrawCommand> &commands, const TwoHalfD::Position &cameraPos);
    void traverse(TwoHalfD::BSPNode *node, std::vector<TwoHalfD::DrawCommand> &commands, const TwoHalfD::Position &cameraPos,
                  const TwoHalfD::XYVectorf &cameraDir);

    // Getters and setters
    void setLevel(const TwoHalfD::Level *level);
    TwoHalfD::Segment &getSegment(int id);

    // BSP optimization
    int findBestPartitioning();

  private:
    const TwoHalfD::Level *m_level;
    std::unique_ptr<TwoHalfD::BSPNode> m_root;
    std::vector<TwoHalfD::Segment> m_segments;
    size_t m_segmentID = 0;
    float m_splitWeight = 3.f;
    int m_startSeed = 0;
    int m_endSeed = 20000;

    void _buildBSPTree(TwoHalfD::BSPNode *node, const std::vector<TwoHalfD::Segment> &inputSegments,
                       std::vector<TwoHalfD::XYVectorf> &currentBoundaryPoints, struct OptimalCostPartitioning &cost, bool saveSegments = true);
    std::pair<std::vector<TwoHalfD::Segment>, std::vector<TwoHalfD::Segment>> _splitSpace(TwoHalfD::BSPNode *node,
                                                                                          const std::vector<TwoHalfD::Segment> &inputSegments,
                                                                                          struct OptimalCostPartitioning &cost,
                                                                                          bool saveSegments = true);
    void _addSegment(TwoHalfD::Segment &&segment, TwoHalfD::BSPNode *node);
    void _insertSprite(TwoHalfD::BSPNode *node, const TwoHalfD::SpriteEntity &sprite, int spriteId);

    // BSP optimization
    float _findIndividualPartitioning(int seed, std::vector<TwoHalfD::Segment> segments);
};
} // namespace TwoHalfD

#endif