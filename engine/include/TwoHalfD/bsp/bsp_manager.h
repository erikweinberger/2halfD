#ifndef BSP_MANAGER_H
#define BSP_MANAGER_H

#include "TwoHalfD/engine_types.h"
#include <cstddef>
#include <memory>
#include <queue>
#include <vector>

namespace TwoHalfD {

class BSPManager {
  public:
    BSPManager() : m_level(nullptr) {}
    BSPManager(const TwoHalfD::Level *level) : m_level(level) {}

    void buildBSPTree();
    TwoHalfD::Segment &getSegment(int id);

    // Traverse logic
    std::vector<int> update(TwoHalfD::Position &cameraPos);
    void traverse(TwoHalfD::BSPNode *node, std::vector<int> &segmentIds, const TwoHalfD::Position &cameraPos);
    void traverse(TwoHalfD::BSPNode *node, std::vector<int> &segmentIds, const TwoHalfD::Position &cameraPos, const TwoHalfD::XYVectorf &cameraDir);

    // Getters and setters
    void setLevel(const TwoHalfD::Level *level);

  private:
    const TwoHalfD::Level *m_level;
    std::unique_ptr<TwoHalfD::BSPNode> m_root;
    std::vector<TwoHalfD::Segment> m_segments;
    size_t m_segmentID = 0;

    void _buildBSPTree(TwoHalfD::BSPNode *node, const std::vector<TwoHalfD::Segment> &inputSegments);
    std::pair<std::vector<TwoHalfD::Segment>, std::vector<TwoHalfD::Segment>> _splitSpace(TwoHalfD::BSPNode *node,
                                                                                          const std::vector<TwoHalfD::Segment> &inputSegments);
    void _addSegment(TwoHalfD::Segment &&segment, TwoHalfD::BSPNode *node);
};
} // namespace TwoHalfD

#endif