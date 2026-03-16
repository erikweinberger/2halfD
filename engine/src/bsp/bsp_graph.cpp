#include "TwoHalfD/bsp/bsp_graph.h"
#include "TwoHalfD/utils/math_util.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>

namespace {
constexpr float BSP_EPSILON = 0.01f;
} // namespace

void TwoHalfD::BSPGraph::build(BSPNode *root, const std::vector<Segment> &segments, float defaultFloorHeight) {
    m_nodes.clear();
    m_nodeIndexMap.clear();
    _collectLeaves(root, defaultFloorHeight);
    _processInternalNode(root, segments);
}

void TwoHalfD::BSPGraph::setDoor(int nodeA, int nodeB, int doorId) {
    for (auto &edge : m_nodes[nodeA].edges) {
        if (edge.targetNodeIndex == nodeB) edge.doorId = doorId;
    }
    for (auto &edge : m_nodes[nodeB].edges) {
        if (edge.targetNodeIndex == nodeA) edge.doorId = doorId;
    }
}

int TwoHalfD::BSPGraph::findNodeForPoint(const XYVectorf &point) const {
    for (int i{}; i < static_cast<int>(m_nodes.size()); ++i) {
        const Polygon &bounds = m_nodes[i].bspNode->bounds;
        int n = static_cast<int>(bounds.size());
        if (n < 3) continue;

        float firstSign = 0.f;
        bool inside = true;
        for (int j{}; j < n; ++j) {
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

std::vector<TwoHalfD::XYVectorf> TwoHalfD::BSPGraph::findPath(const XYVectorf &start, const XYVectorf &end, float entityWidth, float maxHeightDiff,
                                                              float maxDistance) const {
    int startNode = findNodeForPoint(start);
    int endNode = findNodeForPoint(end);
    if (startNode == -1 || endNode == -1) return {};
    if (startNode == endNode) return {start, end};

    int n = static_cast<int>(m_nodes.size());
    std::vector<float> gScore(n, std::numeric_limits<float>::max());
    std::vector<int> cameFromNode(n, -1);
    std::vector<XYVectorf> cameFromPortal(n);

    // {fScore, nodeIndex} — pair sorts by first so priority queue orders by fScore
    std::priority_queue<std::pair<float, int>, std::vector<std::pair<float, int>>, std::greater<>> openSet;

    gScore[startNode] = 0.f;
    float h = (m_nodes[startNode].centroid - m_nodes[endNode].centroid).length();
    openSet.push({h, startNode});

    while (!openSet.empty()) {
        auto [f, current] = openSet.top();
        openSet.pop();

        if (current == endNode) {
            std::vector<XYVectorf> path;
            path.push_back(end);
            int node = current;
            while (cameFromNode[node] != -1) {
                path.push_back(cameFromPortal[node]);
                node = cameFromNode[node];
            }
            path.push_back(start);
            std::reverse(path.begin(), path.end());
            return path;
        }

        if (f > gScore[current] + (m_nodes[current].centroid - m_nodes[endNode].centroid).length())
            continue; // stale entry in open set

        for (const auto &edge : m_nodes[current].edges) {
            if (edge.portalWidth < entityWidth) continue;
            if (edge.heightDiff < -maxHeightDiff) continue; // too high to step up

            float stepCost = (m_nodes[current].centroid - edge.portalMidpoint).length() +
                             (edge.portalMidpoint - m_nodes[edge.targetNodeIndex].centroid).length();
            float tentativeG = gScore[current] + stepCost;

            if (maxDistance > 0.f && tentativeG > maxDistance) continue;
            if (tentativeG >= gScore[edge.targetNodeIndex]) continue;

            gScore[edge.targetNodeIndex] = tentativeG;
            cameFromNode[edge.targetNodeIndex] = current;
            cameFromPortal[edge.targetNodeIndex] = edge.portalMidpoint;

            float newH = (m_nodes[edge.targetNodeIndex].centroid - m_nodes[endNode].centroid).length();
            openSet.push({tentativeG + newH, edge.targetNodeIndex});
        }
    }

    return {};
}

void TwoHalfD::BSPGraph::_collectLeaves(BSPNode *node, float defaultFloorHeight) {
    if (node == nullptr) return;

    if (node->front == nullptr && node->back == nullptr) {
        BSPGraphNode graphNode;
        graphNode.bspNode = node;

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
        for (int i{}; i < n; ++i) {
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
                break;
            }
        }
        return;
    }

    _collectLeavesTouchingSplitter(node->front.get(), splitterP0, splitterDir, result);
    _collectLeavesTouchingSplitter(node->back.get(), splitterP0, splitterDir, result);
}

std::vector<std::pair<float, float>> TwoHalfD::BSPGraph::_getUnblockedIntervals(float tA, float tB, const XYVectorf &splitterP0,
                                                                                const XYVectorf &splitterDir, const std::vector<Segment> &segments) {
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

        if (tw2 <= tA + BSP_EPSILON || tw1 >= tB - BSP_EPSILON) continue;

        wallIntervals.push_back({std::max(tw1, tA), std::min(tw2, tB)});
    }

    std::sort(wallIntervals.begin(), wallIntervals.end());

    std::vector<std::pair<float, float>> unblocked;
    float cursor = tA;
    for (const auto &[wStart, wEnd] : wallIntervals) {
        if (wStart > cursor + BSP_EPSILON) unblocked.push_back({cursor, wStart});
        cursor = std::max(cursor, wEnd);
        if (cursor >= tB - BSP_EPSILON) break;
    }
    if (cursor < tB - BSP_EPSILON) unblocked.push_back({cursor, tB});

    return unblocked;
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

            auto openings = _getUnblockedIntervals(overlapStart, overlapEnd, node->splitterP0, splitterDir, segments);

            float heightA = m_nodes[frontIdx].floorHeight;
            float heightB = m_nodes[backIdx].floorHeight;
            float heightDiff = heightA - heightB;

            for (const auto &[a, b] : openings) {
                XYVectorf edgeStart = node->splitterP0 + splitterDir * a;
                XYVectorf edgeEnd = node->splitterP0 + splitterDir * b;
                XYVectorf mid = {(edgeStart.x + edgeEnd.x) * 0.5f, (edgeStart.y + edgeEnd.y) * 0.5f};

                float portalWidth = b - a;
                m_nodes[frontIdx].edges.push_back({backIdx, edgeStart, edgeEnd, portalWidth, heightA - heightB, mid});
                m_nodes[backIdx].edges.push_back({frontIdx, edgeStart, edgeEnd, portalWidth, heightB - heightA, mid});
            }
        }
    }

    _processInternalNode(node->front.get(), segments);
    _processInternalNode(node->back.get(), segments);
}
