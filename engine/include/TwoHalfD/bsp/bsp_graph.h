#ifndef BSP_GRAPH_H
#define BSP_GRAPH_H

#include "TwoHalfD/types/bsp_types.h"
#include "TwoHalfD/types/math_types.h"

#include <unordered_map>
#include <vector>

namespace TwoHalfD {

struct BSPGraphEdge {
    int targetNodeIndex;
    XYVectorf edgeStart;
    XYVectorf edgeEnd;
    float portalWidth;
    float heightDiff; // sourceNode.floorHeight - targetNode.floorHeight; negative means stepping up
    XYVectorf portalMidpoint;
    int doorId = -1;  // -1 = no door; otherwise references a door entity by id
};

struct BSPGraphNode {
    const BSPNode *bspNode;
    XYVectorf centroid;
    float floorHeight;
    std::vector<BSPGraphEdge> edges;
};

class BSPGraph {
  public:
    void build(BSPNode *root, const std::vector<Segment> &segments, float defaultFloorHeight);
    void setDoor(int nodeA, int nodeB, int doorId);

    int getNodeCount() const {
        return static_cast<int>(m_nodes.size());
    }
    const BSPGraphNode &getNode(int index) const {
        return m_nodes[index];
    }
    BSPGraphNode &getNode(int index) {
        return m_nodes[index];
    }
    int findNodeForPoint(const XYVectorf &point) const;
    int getNodeIndex(const BSPNode *node) const;
    const std::vector<BSPGraphNode> &getNodes() const {
        return m_nodes;
    }

    std::vector<XYVectorf> findPath(const XYVectorf &start, const XYVectorf &end, float entityWidth, float maxHeightDiff, float maxDistance) const;

  private:
    std::vector<BSPGraphNode> m_nodes;
    std::unordered_map<const BSPNode *, int> m_nodeIndexMap;

    void _collectLeaves(BSPNode *node, float defaultFloorHeight);
    void _processInternalNode(BSPNode *node, const std::vector<Segment> &segments);
    void _collectLeavesTouchingSplitter(BSPNode *node, const XYVectorf &splitterP0, const XYVectorf &splitterDir,
                                        std::vector<std::pair<int, std::pair<float, float>>> &result);
    std::vector<std::pair<float, float>> _getUnblockedIntervals(float tA, float tB, const XYVectorf &splitterP0, const XYVectorf &splitterDir,
                                                                const std::vector<Segment> &segments);
};

} // namespace TwoHalfD

#endif
