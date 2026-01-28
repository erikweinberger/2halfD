
#include "TwoHalfD/bsp/bsp_manager.h"
#include "../utils/math_util.h"
#include "TwoHalfD/engine_types.h"
#include <cstddef>
#include <limits>
#include <memory>
#include <vector>

void TwoHalfD::BSPManager::buildBSPTree() {
    m_root = std::make_unique<TwoHalfD::BSPNode>();
    std::vector<TwoHalfD::Segment> segments;
    segments.reserve(m_level->walls.size());
    for (const auto &wall : m_level->walls) {
        segments.push_back({{wall.start.x, wall.start.y}, {wall.end.x, wall.end.y}, &wall});
    }
    _buildBSPTree(m_root.get(), segments);

    return;
}

TwoHalfD::Segment &TwoHalfD::BSPManager::getSegment(int id) {
    return m_segments[id];
}

std::vector<int> TwoHalfD::BSPManager::update(TwoHalfD::Position &cameraPos) {
    std::vector<int> segmentIds;
    TwoHalfD::XYVectorf cameraDir{std::cos(cameraPos.direction), std::sin(cameraPos.direction)};
    traverse(m_root.get(), segmentIds, cameraPos, cameraDir);
    return segmentIds;
}

void TwoHalfD::BSPManager::traverse(TwoHalfD::BSPNode *node, std::vector<int> &segmentIds, const TwoHalfD::Position &cameraPos) {
    if (node == nullptr) return;

    float isInfrontOfCamera = isInfront(cameraPos.pos - node->splitterP0, node->splitterVec);

    if (isInfrontOfCamera > std::numeric_limits<float>::epsilon()) {
        traverse(node->front.get(), segmentIds, cameraPos);

        segmentIds.push_back(node->segmentID);

        traverse(node->back.get(), segmentIds, cameraPos);
    } else {
        traverse(node->back.get(), segmentIds, cameraPos);

        segmentIds.push_back(node->segmentID);

        traverse(node->front.get(), segmentIds, cameraPos);
    }
}

void TwoHalfD::BSPManager::traverse(TwoHalfD::BSPNode *node, std::vector<int> &segmentIds, const TwoHalfD::Position &cameraPos,
                                    const TwoHalfD::XYVectorf &cameraDir) {
    if (node == nullptr) return;

    bool isInfrontOfCamera = isInfront(cameraPos.pos - node->splitterP0, node->splitterVec);

    if (isInfrontOfCamera) {
        traverse(node->back.get(), segmentIds, cameraPos, cameraDir);

        segmentIds.push_back(node->segmentID);

        traverse(node->front.get(), segmentIds, cameraPos, cameraDir);

    } else {
        traverse(node->front.get(), segmentIds, cameraPos, cameraDir);

        segmentIds.push_back(node->segmentID);

        traverse(node->back.get(), segmentIds, cameraPos, cameraDir);
    }
}

// Getters and setters

void TwoHalfD::BSPManager::setLevel(const TwoHalfD::Level *level) {
    m_level = level;
}

/* =============================================================================================================================
 * Private functions
 * =============================================================================================================================
 */
void TwoHalfD::BSPManager::_buildBSPTree(TwoHalfD::BSPNode *node, const std::vector<TwoHalfD::Segment> &inputSegments) {
    if (inputSegments.size() == 0) {
        return;
    }

    auto [frontSegs, backSegs] = _splitSpace(node, inputSegments);

    if (backSegs.size() > 0) {
        node->back = std::make_unique<TwoHalfD::BSPNode>();
        _buildBSPTree(node->back.get(), backSegs);
    }

    if (frontSegs.size() > 0) {
        node->front = std::make_unique<TwoHalfD::BSPNode>();
        _buildBSPTree(node->front.get(), frontSegs);
    }
}

std::pair<std::vector<TwoHalfD::Segment>, std::vector<TwoHalfD::Segment>>
TwoHalfD::BSPManager::_splitSpace(TwoHalfD::BSPNode *node, const std::vector<TwoHalfD::Segment> &inputSegments) {
    auto splitterSeg = inputSegments[0];
    TwoHalfD::XYVectorf v1 = splitterSeg.v1;
    TwoHalfD::XYVectorf v2 = splitterSeg.v2;
    TwoHalfD::XYVectorf splitterVec = TwoHalfD::XYVectorf{vectorBetweenPoints(v1, v2)};

    node->splitterVec = splitterVec;
    node->splitterP0 = v1;
    node->splitterP1 = v2;

    std::vector<TwoHalfD::Segment> frontSegs;
    std::vector<TwoHalfD::Segment> backSegs;

    for (size_t i{1}; i < inputSegments.size(); ++i) {
        XYVectorf segmentVector = vectorBetweenPoints(inputSegments[i].v1, inputSegments[i].v2);
        float numerator = crossProduct2d(inputSegments[i].v1 - v1, splitterVec);
        float denominator = crossProduct2d(splitterVec, segmentVector);

        bool denominatorIsZero = std::abs(denominator) < std::numeric_limits<float>::epsilon();
        bool numeratorIsZero = std::abs(numerator) < std::numeric_limits<float>::epsilon();

        if (denominatorIsZero && numeratorIsZero) {
            frontSegs.push_back(inputSegments[i]);
            continue;
        }

        if (!denominatorIsZero) {
            float intersection = numerator / denominator;

            if (intersection > 0.0f && intersection < 1.0f) {
                TwoHalfD::XYVectorf intersectionPoint{inputSegments[i].v1 + intersection * segmentVector};

                const float midPointWallRatio = distanceBetweenPoints(intersectionPoint, inputSegments[i].wall->start) /
                                                distanceBetweenPoints(inputSegments[i].wall->start, inputSegments[i].wall->end);
                TwoHalfD::Segment rSegment{
                    {inputSegments[i].v1}, {intersectionPoint}, inputSegments[i].wall, inputSegments[i].wallRatioStart, midPointWallRatio};
                TwoHalfD::Segment lSegment{
                    {intersectionPoint}, {inputSegments[i].v2}, inputSegments[i].wall, midPointWallRatio, inputSegments[i].wallRatioEnd};

                if (numerator > 0) {
                    std::swap(rSegment, lSegment);
                }

                frontSegs.push_back(rSegment);
                backSegs.push_back(lSegment);
                continue;
            }
        }

        if (numerator < 0 || (numeratorIsZero && denominator > 0)) {
            frontSegs.push_back(inputSegments[i]);
        } else if (numerator > 0 || (numeratorIsZero && denominator < 0)) {
            backSegs.push_back(inputSegments[i]);
        }
    }

    _addSegment(std::move(splitterSeg), node);
    return {frontSegs, backSegs};
}

void TwoHalfD::BSPManager::_addSegment(TwoHalfD::Segment &&segment, TwoHalfD::BSPNode *node) {
    m_segments.push_back(std::move(segment));
    node->segmentID = m_segmentID;

    ++m_segmentID;
}
