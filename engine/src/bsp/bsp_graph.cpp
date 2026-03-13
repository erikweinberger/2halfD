#include "TwoHalfD/bsp/bsp_graph.h"
#include "TwoHalfD/utils/math_util.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr float BSP_EPSILON = 0.01f;
constexpr float HEIGHT_THRESHOLD = 30.f;
} // namespace

// ============================================================
// Public API
// ============================================================

void TwoHalfD::BSPGraph::build(BSPNode *root, const std::vector<Segment> &segments, float defaultFloorHeight) {
    m_nodes.clear();
    m_nodeIndexMap.clear();

    // Phase 1: collect all leaf nodes, cache centroid and floor height
    _collectLeaves(root, defaultFloorHeight);

    // Phase 2: traverse internal nodes to find adjacencies across each splitter
    _processInternalNode(root, segments);
}

int TwoHalfD::BSPGraph::findNodeForPoint(const XYVectorf &point) const {
    for (int i = 0; i < static_cast<int>(m_nodes.size()); ++i) {
        const Polygon &bounds = m_nodes[i].bspNode->bounds;
        int n = static_cast<int>(bounds.size());
        if (n < 3) continue;

        // Point-in-convex-polygon: all cross products of (edge, toPoint) must share the same sign
        float firstSign = 0.f;
        bool inside = true;
        for (int j = 0; j < n; ++j) {
            XYVectorf edge = bounds[(j + 1) % n] - bounds[j];
            XYVectorf toPoint = point - bounds[j];
            float cross = crossProduct2d(edge, toPoint);
            if (j == 0) {
                firstSign = (cross >= 0.f) ? 1.f : -1.f;
            } else if (cross != 0.f && ((cross > 0.f) ? 1.f : -1.f) != firstSign) {
                inside = false;
                break;
            }
        }
        if (inside) return i;
    }
    return -1;
}

int TwoHalfD::BSPGraph::getNodeIndex(const BSPNode *node) const {
    auto it = m_nodeIndexMap.find(node);
    return (it != m_nodeIndexMap.end()) ? it->second : -1;
}

// ============================================================
// Private helpers
// ============================================================

void TwoHalfD::BSPGraph::_collectLeaves(BSPNode *node, float defaultFloorHeight) {
    if (node == nullptr) return;

    if (node->front == nullptr && node->back == nullptr) {
        BSPGraphNode graphNode;
        graphNode.bspNode = node;

        // Centroid = average of bounds vertices
        XYVectorf centroid{0.f, 0.f};
        for (const auto &v : node->bounds) {
            centroid.x += v.x;
            centroid.y += v.y;
        }
        if (!node->bounds.empty()) {
            float inv = 1.f / static_cast<float>(node->bounds.size());
            centroid.x *= inv;
            centroid.y *= inv;
        }
        graphNode.centroid = centroid;

        graphNode.floorHeight = (node->floorSection != nullptr) ? node->floorSection->height : defaultFloorHeight;

        int index = static_cast<int>(m_nodes.size());
        m_nodeIndexMap[node] = index;
        m_nodes.push_back(std::move(graphNode));
        return;
    }

    _collectLeaves(node->front.get(), defaultFloorHeight);
    _collectLeaves(node->back.get(), defaultFloorHeight);
}

void TwoHalfD::BSPGraph::_collectLeavesTouchingSplitter(BSPNode *node, const XYVectorf &splitterP0, const XYVectorf &splitterDir,
                                                         std::vector<std::pair<int, std::pair<float, float>>> &result) {
    if (node == nullptr) return;

    if (node->front == nullptr && node->back == nullptr) {
        const Polygon &bounds = node->bounds;
        int n = static_cast<int>(bounds.size());
        for (int i = 0; i < n; ++i) {
            const XYVectorf &v1 = bounds[i];
            const XYVectorf &v2 = bounds[(i + 1) % n];

            float d1 = std::abs(crossProduct2d(v1 - splitterP0, splitterDir));
            float d2 = std::abs(crossProduct2d(v2 - splitterP0, splitterDir));

            if (d1 < BSP_EPSILON && d2 < BSP_EPSILON) {
                float t1 = dotProduct(v1 - splitterP0, splitterDir);
                float t2 = dotProduct(v2 - splitterP0, splitterDir);
                float tMin = std::min(t1, t2);
                float tMax = std::max(t1, t2);

                if (tMax - tMin > BSP_EPSILON) {
                    auto it = m_nodeIndexMap.find(node);
                    if (it != m_nodeIndexMap.end()) {
                        result.push_back({it->second, {tMin, tMax}});
                    }
                }
                break; // convex polygon has at most one edge on any line
            }
        }
        return;
    }

    _collectLeavesTouchingSplitter(node->front.get(), splitterP0, splitterDir, result);
    _collectLeavesTouchingSplitter(node->back.get(), splitterP0, splitterDir, result);
}

bool TwoHalfD::BSPGraph::_isEdgeWallBlocked(float tA, float tB, const XYVectorf &splitterP0, const XYVectorf &splitterDir,
                                              const std::vector<Segment> &segments) {
    if (tA > tB) std::swap(tA, tB);

    std::vector<std::pair<float, float>> wallIntervals;

    for (const auto &seg : segments) {
        if (!seg.isWall()) continue;

        float d1 = std::abs(crossProduct2d(seg.v1 - splitterP0, splitterDir));
        float d2 = std::abs(crossProduct2d(seg.v2 - splitterP0, splitterDir));
        if (d1 > BSP_EPSILON || d2 > BSP_EPSILON) continue;

        float tw1 = dotProduct(seg.v1 - splitterP0, splitterDir);
        float tw2 = dotProduct(seg.v2 - splitterP0, splitterDir);
        if (tw1 > tw2) std::swap(tw1, tw2);

        // Skip if no overlap with [tA, tB]
        if (tw2 <= tA || tw1 >= tB) continue;

        wallIntervals.push_back({std::max(tw1, tA), std::min(tw2, tB)});
    }

    if (wallIntervals.empty()) return false;

    std::sort(wallIntervals.begin(), wallIntervals.end());

    float coveredUpTo = tA;
    for (const auto &[start, end] : wallIntervals) {
        if (start > coveredUpTo + BSP_EPSILON) return false; // gap in coverage
        coveredUpTo = std::max(coveredUpTo, end);
    }

    return coveredUpTo >= tB - BSP_EPSILON;
}

void TwoHalfD::BSPGraph::_processInternalNode(BSPNode *node, const std::vector<Segment> &segments) {
    if (node == nullptr) return;
    if (node->front == nullptr && node->back == nullptr) return;

    XYVectorf splitterDir = node->splitterVec.normalized();
    if (splitterDir.length() < 1e-6f) {
        _processInternalNode(node->front.get(), segments);
        _processInternalNode(node->back.get(), segments);
        return;
    }

    std::vector<std::pair<int, std::pair<float, float>>> frontLeaves, backLeaves;
    _collectLeavesTouchingSplitter(node->front.get(), node->splitterP0, splitterDir, frontLeaves);
    _collectLeavesTouchingSplitter(node->back.get(), node->splitterP0, splitterDir, backLeaves);

    for (const auto &[frontIdx, frontInterval] : frontLeaves) {
        for (const auto &[backIdx, backInterval] : backLeaves) {
            float overlapStart = std::max(frontInterval.first, backInterval.first);
            float overlapEnd = std::min(frontInterval.second, backInterval.second);

            if (overlapEnd - overlapStart <= BSP_EPSILON) continue;

            if (_isEdgeWallBlocked(overlapStart, overlapEnd, node->splitterP0, splitterDir, segments)) continue;

            XYVectorf edgeStart = node->splitterP0 + splitterDir * overlapStart;
            XYVectorf edgeEnd = node->splitterP0 + splitterDir * overlapEnd;
            float edgeLength = overlapEnd - overlapStart;

            float heightA = m_nodes[frontIdx].floorHeight;
            float heightB = m_nodes[backIdx].floorHeight;
            float heightDiff = heightA - heightB;

            if (std::abs(heightDiff) <= HEIGHT_THRESHOLD) {
                m_nodes[frontIdx].edges.push_back({backIdx, edgeStart, edgeEnd, edgeLength, false});
                m_nodes[backIdx].edges.push_back({frontIdx, edgeStart, edgeEnd, edgeLength, false});
            } else if (heightDiff > HEIGHT_THRESHOLD) {
                // A is higher — can only fall from A down to B
                m_nodes[frontIdx].edges.push_back({backIdx, edgeStart, edgeEnd, edgeLength, true});
            } else {
                // B is higher — can only fall from B down to A
                m_nodes[backIdx].edges.push_back({frontIdx, edgeStart, edgeEnd, edgeLength, true});
            }
        }
    }

    _processInternalNode(node->front.get(), segments);
    _processInternalNode(node->back.get(), segments);
}
