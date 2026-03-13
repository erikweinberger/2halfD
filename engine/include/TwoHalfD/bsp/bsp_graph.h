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
    float edgeLength;
    bool isDropOnly; // true if one-way fall-down edge
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

    int getNodeCount() const { return static_cast<int>(m_nodes.size()); }
    const BSPGraphNode &getNode(int index) const { return m_nodes[index]; }
    int findNodeForPoint(const XYVectorf &point) const;
    int getNodeIndex(const BSPNode *node) const;
    const std::vector<BSPGraphNode> &getNodes() const { return m_nodes; }

  private:
    std::vector<BSPGraphNode> m_nodes;
    std::unordered_map<const BSPNode *, int> m_nodeIndexMap;

    void _collectLeaves(BSPNode *node, float defaultFloorHeight);
    void _processInternalNode(BSPNode *node, const std::vector<Segment> &segments);
    void _collectLeavesTouchingSplitter(BSPNode *node, const XYVectorf &splitterP0, const XYVectorf &splitterDir,
                                        std::vector<std::pair<int, std::pair<float, float>>> &result);
    bool _isEdgeWallBlocked(float tA, float tB, const XYVectorf &splitterP0, const XYVectorf &splitterDir,
                             const std::vector<Segment> &segments);
};

} // namespace TwoHalfD

#endif
