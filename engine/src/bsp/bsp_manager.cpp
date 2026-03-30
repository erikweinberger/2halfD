#include "TwoHalfD/bsp/bsp_manager.h"
#include "TwoHalfD/engine_types.h"
#include "TwoHalfD/utils/math_util.h"
#include <SFML/Window/Cursor.hpp>
#include <cstddef>
#include <iostream>
#include <limits>
#include <memory>
#include <queue>
#include <random>
#include <thread>
#include <unordered_map>
#include <vector>

void TwoHalfD::BSPManager::init(std::vector<Wall> walls, std::unordered_map<int, FloorSection> floorSections, float defaultFloorHeight,
                                int defaultFloorTextureId, int seed) {
    m_walls = std::move(walls);
    m_floorSections = std::move(floorSections);
    m_defaultFloorHeight = defaultFloorHeight;
    m_defaultFloorTextureId = defaultFloorTextureId;
    m_seed = seed;
}

void TwoHalfD::BSPManager::buildBSPTree() {
    int seed = 1; // m_seed;
    std::cout << "Seed for BSP: " << seed << std::endl;
    if (seed == -1) {
        seed = findBestPartitioning();
    }

    m_root = std::make_unique<TwoHalfD::BSPNode>();
    std::vector<TwoHalfD::Segment> segments;
    if (m_walls.size() == 0) {
        return;
    }
    segments.reserve(m_walls.size());
    for (const auto &wall : m_walls) {
        segments.push_back({{wall.start.x, wall.start.y}, {wall.end.x, wall.end.y}, &wall, nullptr});
    }

    for (const auto &floorSectionIt : m_floorSections) {
        const auto &floorSection = floorSectionIt.second;
        size_t n = floorSection.vertices.size();
        for (size_t i{}; i < n; ++i) {
            const auto &currVert = floorSection.vertices[i];
            const auto &nextVert = floorSection.vertices[(i + 1) % n];

            segments.push_back({currVert, nextVert, nullptr, &floorSection});
        }
    }

    TwoHalfD::Polygon initialBounds = _getInitialBounds(segments);

    std::mt19937 rng(seed);
    std::shuffle(segments.begin(), segments.end(), rng);
    OptimalCostPartitioning cost{0, 0, 0};
    _buildBSPTree(m_root.get(), segments, initialBounds, -1, cost);

    return;
}

std::unordered_map<int, float> TwoHalfD::BSPManager::insertSprites(const std::unordered_map<int, SpriteEntity> &entities) {
    std::unordered_map<int, float> heightStarts;
    for (const auto &[entityId, entity] : entities) {
        m_spritePositions[entityId] = entity.pos.pos;
        float h = _insertSprite(m_root.get(), entityId, entity.pos.pos);
        heightStarts[entityId] = h;
    }
    return heightStarts;
}

float TwoHalfD::BSPManager::moveSprite(int entityId, TwoHalfD::XYVectorf newPos) {
    auto nodeIt = m_spriteNodeMap.find(entityId);
    if (nodeIt != m_spriteNodeMap.end()) {
        nodeIt->second->spriteIds.erase(entityId);
    }
    m_spritePositions[entityId] = newPos;
    return _insertSprite(m_root.get(), entityId, newPos);
}

TwoHalfD::Segment &TwoHalfD::BSPManager::getSegment(int id) {
    return m_segments[id];
}

const std::vector<TwoHalfD::Wall> &TwoHalfD::BSPManager::getWalls() const {
    return m_walls;
}

const std::unordered_map<int, TwoHalfD::FloorSection> &TwoHalfD::BSPManager::getFloorSections() const {
    return m_floorSections;
}

std::pair<std::vector<TwoHalfD::DrawCommand>, std::unordered_set<int>> TwoHalfD::BSPManager::update(TwoHalfD::Position &cameraPos) {
    TwoHalfD::XYVectorf cameraDir{std::cos(cameraPos.direction), std::sin(cameraPos.direction)};
    std::vector<TwoHalfD::DrawCommand> commands;
    std::unordered_set<int> floorSectionIds;
    if (m_root == nullptr || m_segments.size() == 0) {
        return {commands, floorSectionIds};
    }
    traverse(m_root.get(), commands, floorSectionIds, cameraPos, cameraDir);

    return {commands, floorSectionIds};
}

void TwoHalfD::BSPManager::traverse(TwoHalfD::BSPNode *node, std::vector<TwoHalfD::DrawCommand> &commands, std::unordered_set<int> &floorSectionIds,
                                    const TwoHalfD::Position &cameraPos) {
    if (node == nullptr) return;

    bool isInfrontOfCamera = isInfront(cameraPos.pos - node->splitterP0, node->splitterVec);

    if (node->front == nullptr && node->back == nullptr) {

        auto cmp = [](const auto &a, const auto &b) { return a.first > b.first; };
        std::priority_queue<std::pair<float, int>, std::vector<std::pair<float, int>>, decltype(cmp)> spriteOrderedDistance(cmp);

        for (const auto &entityId : node->spriteIds) {
            auto posIt = m_spritePositions.find(entityId);
            if (posIt == m_spritePositions.end()) continue;
            float distance = (posIt->second - cameraPos.pos).length();
            spriteOrderedDistance.push({distance, entityId});
        }

        if (!isInfrontOfCamera) {
            while (!spriteOrderedDistance.empty()) {
                commands.push_back(DrawCommand::makeSprite(spriteOrderedDistance.top().second));
                spriteOrderedDistance.pop();
            }
            commands.push_back(DrawCommand::makeSegment(static_cast<int>(node->segmentID)));
        } else {
            commands.push_back(DrawCommand::makeSegment(static_cast<int>(node->segmentID)));
            while (!spriteOrderedDistance.empty()) {
                commands.push_back(DrawCommand::makeSprite(spriteOrderedDistance.top().second));
                spriteOrderedDistance.pop();
            }
        }
        return;
    }

    if (isInfrontOfCamera) {
        traverse(node->back.get(), commands, floorSectionIds, cameraPos);

        commands.push_back(DrawCommand::makeSegment(node->segmentID));
        traverse(node->front.get(), commands, floorSectionIds, cameraPos);

    } else {
        traverse(node->front.get(), commands, floorSectionIds, cameraPos);

        commands.push_back(DrawCommand::makeSegment(node->segmentID));

        traverse(node->back.get(), commands, floorSectionIds, cameraPos);
    }
}

void TwoHalfD::BSPManager::traverse(TwoHalfD::BSPNode *node, std::vector<TwoHalfD::DrawCommand> &commands, std::unordered_set<int> &floorSectionIds,
                                    const TwoHalfD::Position &cameraPos, const TwoHalfD::XYVectorf &cameraDir) {
    if (node == nullptr) return;

    bool isInfrontOfCamera = isInfront(cameraPos.pos - node->splitterP0, node->splitterVec);

    if (node->front == nullptr && node->back == nullptr) {
        // std::cout << "At leaf node: " << node->floorSectionId << '\n';
        if (node->floorSection != nullptr) {
            commands.push_back(DrawCommand::makeFloorSection(node->floorSection.get()));
        }

        auto cmp = [](const auto &a, const auto &b) { return a.first > b.first; };
        std::priority_queue<std::pair<float, int>, std::vector<std::pair<float, int>>, decltype(cmp)> spriteOrderedDistance(cmp);

        for (const auto &entityId : node->spriteIds) {
            auto posIt = m_spritePositions.find(entityId);
            if (posIt == m_spritePositions.end()) continue;
            float distance = (posIt->second - cameraPos.pos).length();
            spriteOrderedDistance.push({distance, entityId});
        }

        if (!isInfrontOfCamera) {
            while (!spriteOrderedDistance.empty()) {
                commands.push_back(DrawCommand::makeSprite(spriteOrderedDistance.top().second));
                spriteOrderedDistance.pop();
            }
        } else {
            while (!spriteOrderedDistance.empty()) {
                commands.push_back(DrawCommand::makeSprite(spriteOrderedDistance.top().second));
                spriteOrderedDistance.pop();
            }
        }
        return;
    }

    if (isInfrontOfCamera) {
        traverse(node->back.get(), commands, floorSectionIds, cameraPos, cameraDir);

        commands.push_back(DrawCommand::makeSegment(node->segmentID));

        traverse(node->front.get(), commands, floorSectionIds, cameraPos, cameraDir);

    } else {
        traverse(node->front.get(), commands, floorSectionIds, cameraPos, cameraDir);

        commands.push_back(DrawCommand::makeSegment(node->segmentID));

        traverse(node->back.get(), commands, floorSectionIds, cameraPos, cameraDir);
    }
}

TwoHalfD::Path TwoHalfD::BSPManager::findPath(const TwoHalfD::XYVectorf &start, const TwoHalfD::XYVectorf &end, float entityWidth,
                                              float maxHeightDiff, float maxDistance) {
    auto path = m_graph.findPath(start, end, entityWidth, maxHeightDiff, maxDistance);
    if (path.size() <= 2) return path;
    return _smoothPath(path, entityWidth, maxHeightDiff);
}

TwoHalfD::Path TwoHalfD::BSPManager::_smoothPath(const TwoHalfD::Path &path, float entityWidth, float maxHeightDiff) {
    TwoHalfD::Path smoothed;
    smoothed.push_back(path[0]);
    size_t current = 0;

    while (current < path.size() - 1) {
        size_t farthest = current + 1;
        for (size_t i = path.size() - 1; i > current + 1; --i) {
            if (_hasLineOfSight(path[current], path[i], entityWidth, maxHeightDiff)) {
                farthest = i;
                break;
            }
        }
        smoothed.push_back(path[farthest]);
        current = farthest;
    }

    return smoothed;
}

bool TwoHalfD::BSPManager::_hasLineOfSight(const TwoHalfD::XYVectorf &a, const TwoHalfD::XYVectorf &b, [[maybe_unused]] float entityWidth,
                                           float maxHeightDiff) {
    TwoHalfD::XYVectorf dir = b - a;
    if (dir.length() < 0.001f) return true;

    for (const auto &seg : m_segments) {
        TwoHalfD::XYVectorf d1 = seg.v1 - a;
        TwoHalfD::XYVectorf d2 = seg.v2 - a;

        float cross1 = crossProduct2d(dir, d1);
        float cross2 = crossProduct2d(dir, d2);
        if (cross1 * cross2 > 0.f) continue;

        TwoHalfD::XYVectorf segDir = seg.v2 - seg.v1;
        float cross3 = crossProduct2d(segDir, a - seg.v1);
        float cross4 = crossProduct2d(segDir, b - seg.v1);
        if (cross3 * cross4 > 0.f) continue;

        // Line crosses this segment
        if (seg.isWall()) return false;

        // Floor boundary — block if height diff is too large
        if (seg.isFloorBoundary()) {
            float floorHeight = seg.floorSection->height;
            float defaultHeight = m_defaultFloorHeight;

            // The floor section is on one side, default floor on the other
            float heightDiff = std::abs(floorHeight - defaultHeight);
            if (heightDiff > maxHeightDiff) return false;
        }
    }

    return true;
}

// Getters
void TwoHalfD::BSPManager::buildGraph() {
    m_graph.build(m_root.get(), m_segments, m_defaultFloorHeight);
}

TwoHalfD::BSPGraph &TwoHalfD::BSPManager::getGraph() {
    return m_graph;
}

int TwoHalfD::BSPManager::findBestPartitioning() {
    std::vector<TwoHalfD::Segment> segments;
    segments.reserve(m_walls.size());
    for (const auto &wall : m_walls) {
        segments.push_back({{wall.start.x, wall.start.y}, {wall.end.x, wall.end.y}, &wall, nullptr});
    }

    for (const auto &floorSectionIt : m_floorSections) {
        const auto &floorSection = floorSectionIt.second;
        size_t n = floorSection.vertices.size();
        for (size_t i{}; i < n; ++i) {
            const auto &currVert = floorSection.vertices[i];
            const auto &nextVert = floorSection.vertices[(i + 1) % n];

            segments.push_back({currVert, nextVert, nullptr, &floorSection});
        }
    }

    const unsigned int maxThreads = std::thread::hardware_concurrency();
    const unsigned int numThreads = std::min(maxThreads, 4u);
    const int totalSeeds = m_endSeed - m_startSeed;
    const int seedsPerThread = totalSeeds / numThreads;

    std::vector<std::thread> threads;
    std::vector<std::pair<int, float>> threadResults(numThreads, {-1, std::numeric_limits<float>::max()});

    for (unsigned int t = 0; t < numThreads; ++t) {
        int startSeed = m_startSeed + t * seedsPerThread;
        int endSeed = (t == numThreads - 1) ? m_endSeed : startSeed + seedsPerThread;

        threads.emplace_back([this, &segments, &threadResults, t, startSeed, endSeed]() {
            int bestSeed = -1;
            float lowestScore = std::numeric_limits<float>::max();

            for (int seed = startSeed; seed < endSeed; ++seed) {
                float score = _findIndividualPartitioning(seed, segments);
                if (score < lowestScore) {
                    lowestScore = score;
                    bestSeed = seed;
                }
            }

            threadResults[t] = {bestSeed, lowestScore};
        });
    }

    for (auto &thread : threads) {
        thread.join();
    }

    int bestSeed = -1;
    float lowestScore = std::numeric_limits<float>::max();
    for (const auto &[seed, score] : threadResults) {
        if (score < lowestScore) {
            lowestScore = score;
            bestSeed = seed;
        }
    }
    std::cout << "BSP Tree built with cost " << lowestScore << std::endl;
    return bestSeed;
}

float TwoHalfD::BSPManager::_findIndividualPartitioning(int seed, std::vector<TwoHalfD::Segment> segments) {
    std::mt19937 rng(seed);
    std::shuffle(segments.begin(), segments.end(), rng);

    TwoHalfD::OptimalCostPartitioning cost{0, 0, 0};
    TwoHalfD::BSPNode rootNode;
    TwoHalfD::Polygon initialBounds = _getInitialBounds(segments);
    _buildBSPTree(&rootNode, segments, initialBounds, -1, cost, false);

    float score = std::abs(cost.numBack - cost.numFront) + (cost.splitCount * m_splitWeight);
    return score;
}

std::vector<TwoHalfD::Segment> TwoHalfD::BSPManager::findSegmentIntersection(const TwoHalfD::XYVectorf &p1, const float radius) {
    std::vector<TwoHalfD::Segment> intersectedSegments;

    _findSegmentIntersections(p1, radius, m_root.get(), intersectedSegments);

    return intersectedSegments;
}

std::vector<std::pair<TwoHalfD::XYVectorf, TwoHalfD::XYVectorf>> TwoHalfD::BSPManager::findCollisions(const TwoHalfD::XYVectorf &pos, float radius,
                                                                                                      float cameraHeightStart) {
    auto intersectedSegments = findSegmentIntersection(pos, radius);

    std::vector<std::pair<TwoHalfD::XYVectorf, TwoHalfD::XYVectorf>> result;
    for (const auto &segment : intersectedSegments) {
        if (segment.isWall()) {
            result.push_back({segment.v1, segment.v2});
        } else if (segment.isFloorBoundary()) {
            if (segment.floorSection->height - cameraHeightStart > 30.f) {
                result.push_back({segment.v1, segment.v2});
            }
        }
    }
    return result;
}

TwoHalfD::BSPNode *TwoHalfD::BSPManager::findConvexSection(const TwoHalfD::XYVectorf &point) {
    return _findConvexSection(point, m_root.get());
}

/* =============================================================================================================================
 * Private functions
 * =============================================================================================================================
 */
void TwoHalfD::BSPManager::_buildBSPTree(TwoHalfD::BSPNode *node, const std::vector<TwoHalfD::Segment> &inputSegments, Polygon bounds,
                                         int floorSectionId, struct OptimalCostPartitioning &cost, bool saveSegments) {
    if (inputSegments.size() == 0) {
        return;
    }
    const auto &splitterSeg = inputSegments[0];
    auto [frontBounds, backBounds] = _splitConvexShape(bounds, splitterSeg);
    int frontSectionFloorId = floorSectionId;
    int backSectionFloorId = floorSectionId;
    if (splitterSeg.isFloorBoundary() && splitterSeg.floorSection->isCCW) {
        backSectionFloorId = splitterSeg.floorSection->id;
        frontSectionFloorId = -1;
    } else if (splitterSeg.isFloorBoundary() && !splitterSeg.floorSection->isCCW) {
        frontSectionFloorId = splitterSeg.floorSection->id;
        backSectionFloorId = -1;
    }
    if (splitterSeg.isWall() && floorSectionId != -1) {
        auto floorIt = m_floorSections.find(floorSectionId);
        if (floorIt != m_floorSections.end()) {
            auto &floorPoly = floorIt->second.vertices;

            bool anyFront = false;
            bool anyBack = false;

            for (const auto &vertex : floorPoly) {
                bool side = isInfront(vertex - splitterSeg.v1, splitterSeg.v2 - splitterSeg.v1);
                if (side) anyFront = true;
                else anyBack = true;
            }

            if (anyFront && anyBack) {
                // Wall cuts through floor section - both sides keep the floor id
                frontSectionFloorId = floorSectionId;
                backSectionFloorId = floorSectionId;
            } else if (anyFront) {
                // Floor section entirely on front side
                frontSectionFloorId = floorSectionId;
                backSectionFloorId = -1;
            } else {
                // Floor section entirely on back side
                frontSectionFloorId = -1;
                backSectionFloorId = floorSectionId;
            }
        }
    }
    auto [frontSegs, backSegs] = _splitSpace(node, inputSegments, cost, saveSegments);

    if (backSegs.size() > 0) {
        node->back = std::make_unique<TwoHalfD::BSPNode>();
        cost.numBack += 1;
        _buildBSPTree(node->back.get(), backSegs, backBounds, backSectionFloorId, cost, saveSegments);
    } else if (saveSegments) {
        node->back = std::make_unique<TwoHalfD::BSPNode>();
        node->back->bounds = backBounds;
        auto floorSectionIt = m_floorSections.find(backSectionFloorId);
        if (floorSectionIt != m_floorSections.end() && backSectionFloorId != -1) {
            auto floorSection = TwoHalfD::FloorSection{backBounds,
                                                       floorSectionIt->second.floorTextureStart,
                                                       floorSectionId,
                                                       floorSectionIt->second.textureId,
                                                       floorSectionIt->second.height,
                                                       floorSectionIt->second.isCCW};
            node->back->floorSection = std::make_unique<TwoHalfD::FloorSection>(floorSection);
        }
    }

    if (frontSegs.size() > 0) {
        node->front = std::make_unique<TwoHalfD::BSPNode>();
        cost.numFront += 1;
        _buildBSPTree(node->front.get(), frontSegs, frontBounds, frontSectionFloorId, cost, saveSegments);
    } else if (saveSegments) {
        node->front = std::make_unique<TwoHalfD::BSPNode>();
        node->front->bounds = frontBounds;

        auto floorSectionIt = m_floorSections.find(frontSectionFloorId);
        if (floorSectionIt != m_floorSections.end() && frontSectionFloorId != -1) {
            auto floorSection = TwoHalfD::FloorSection{frontBounds,
                                                       floorSectionIt->second.floorTextureStart,
                                                       floorSectionId,
                                                       floorSectionIt->second.textureId,
                                                       floorSectionIt->second.height,
                                                       floorSectionIt->second.isCCW};
            node->front->floorSection = std::make_unique<TwoHalfD::FloorSection>(floorSection);
        }
    }
}

std::pair<std::vector<TwoHalfD::Segment>, std::vector<TwoHalfD::Segment>>
TwoHalfD::BSPManager::_splitSpace(TwoHalfD::BSPNode *node, const std::vector<TwoHalfD::Segment> &inputSegments, struct OptimalCostPartitioning &cost,
                                  bool saveSegments) {
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

        bool denominatorIsZero = std::abs(denominator) < BSP_EPSILON;
        bool numeratorIsZero = std::abs(numerator) < BSP_EPSILON;

        if (denominatorIsZero && numeratorIsZero) {
            frontSegs.push_back(inputSegments[i]);
            continue;
        }

        if (!denominatorIsZero) {
            float intersection = numerator / denominator;

            if (intersection > 0.0f && intersection < 1.0f) {
                TwoHalfD::XYVectorf intersectionPoint{inputSegments[i].v1 + intersection * segmentVector};

                if (inputSegments[i].isWall()) {
                    const float midPointWallRatio = distanceBetweenPoints(intersectionPoint, inputSegments[i].wall->start) /
                                                    distanceBetweenPoints(inputSegments[i].wall->start, inputSegments[i].wall->end);
                    TwoHalfD::Segment rSegment{{inputSegments[i].v1},           {intersectionPoint}, inputSegments[i].wall, nullptr,
                                               inputSegments[i].wallRatioStart, midPointWallRatio};
                    TwoHalfD::Segment lSegment{{intersectionPoint}, {inputSegments[i].v2},        inputSegments[i].wall, nullptr,
                                               midPointWallRatio,   inputSegments[i].wallRatioEnd};

                    if (numerator > 0) {
                        std::swap(rSegment, lSegment);
                    }
                    cost.splitCount += 1;
                    frontSegs.push_back(rSegment);
                    backSegs.push_back(lSegment);
                    continue;
                } else if (inputSegments[i].isFloorBoundary()) {
                    TwoHalfD::Segment rSegment{{inputSegments[i].v1}, {intersectionPoint}, nullptr, inputSegments[i].floorSection, 0.f, 1.f};
                    TwoHalfD::Segment lSegment{{intersectionPoint}, {inputSegments[i].v2}, nullptr, inputSegments[i].floorSection, 0.f, 1.f};
                    if (numerator > 0) {
                        std::swap(rSegment, lSegment);
                    }
                    frontSegs.push_back(rSegment);
                    backSegs.push_back(lSegment);
                    continue;
                }
            }
        }

        if (numerator < 0 || (numeratorIsZero && denominator > 0)) {
            frontSegs.push_back(inputSegments[i]);
        } else if (numerator > 0 || (numeratorIsZero && denominator < 0)) {
            backSegs.push_back(inputSegments[i]);
        }
    }

    if (saveSegments) {
        _addSegment(std::move(splitterSeg), node);
    }
    return {frontSegs, backSegs};
}

float TwoHalfD::BSPManager::_insertSprite(TwoHalfD::BSPNode *node, int entityId, TwoHalfD::XYVectorf pos) {
    if (node == nullptr) return 0.f;

    // If this is a leaf node, add the sprite here
    if (node->front == nullptr && node->back == nullptr) {
        float spriteHeight = 0.f;
        if (node->floorSection != nullptr) {
            spriteHeight = node->floorSection->height;
        } else if (m_defaultFloorTextureId != -1) {
            spriteHeight = m_defaultFloorHeight;
        }
        node->spriteIds.insert(entityId);
        m_spriteNodeMap[entityId] = node;
        return spriteHeight;
    }

    float isInfrontOfSplit = isInfront(pos - node->splitterP0, node->splitterVec);

    if (isInfrontOfSplit > std::numeric_limits<float>::epsilon()) {
        return _insertSprite(node->front.get(), entityId, pos);
    } else {
        return _insertSprite(node->back.get(), entityId, pos);
    }
}

std::pair<TwoHalfD::Polygon, TwoHalfD::Polygon> TwoHalfD::BSPManager::_splitConvexShape(const std::vector<TwoHalfD::XYVectorf> &vertices,
                                                                                        const TwoHalfD::Segment &splitter) {
    TwoHalfD::Polygon frontVertices;
    TwoHalfD::Polygon backVertices;

    for (size_t i{}; i < vertices.size(); ++i) {
        const auto &currVert = vertices[i];
        const auto &nextVert = vertices[(i + 1) % vertices.size()];
        bool sideCurr = isInfront(currVert - splitter.v1, splitter.v2 - splitter.v1);
        bool sideNext = isInfront(nextVert - splitter.v1, splitter.v2 - splitter.v1);

        if (sideCurr) frontVertices.push_back(currVert);
        if (!sideCurr) backVertices.push_back(currVert);

        if ((sideCurr && !sideNext) || (!sideCurr && sideNext)) {
            XYVectorf ip = computeLineIntersection(currVert, nextVert, splitter.v1, splitter.v2);
            frontVertices.push_back(ip);
            backVertices.push_back(ip);
        }
    }

    return {{frontVertices}, {backVertices}};
}

void TwoHalfD::BSPManager::_addSegment(TwoHalfD::Segment &&segment, TwoHalfD::BSPNode *node) {
    m_segments.push_back(std::move(segment));
    node->segmentID = m_segmentID;

    ++m_segmentID;
}

TwoHalfD::Polygon TwoHalfD::BSPManager::_getInitialBounds(const std::vector<TwoHalfD::Segment> &segments) {
    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();

    for (const auto &segment : segments) {
        minX = std::min({minX, segment.v1.x, segment.v2.x});
        minY = std::min({minY, segment.v1.y, segment.v2.y});
        maxX = std::max({maxX, segment.v1.x, segment.v2.x});
        maxY = std::max({maxY, segment.v1.y, segment.v2.y});
    }

    TwoHalfD::Polygon initialBounds = {
        {minX - 100.f, minY - 100.f}, // bottom-left
        {maxX + 100.f, minY - 100.f}, // bottom-right
        {maxX + 100.f, maxY + 100.f}, // top-right
        {minX - 100.f, maxY + 100.f}, // top-left
    };

    std::cout << "Initial bounds:\n";
    for (const auto &vertex : initialBounds) {
        std::cout << vertex.x << ", " << vertex.y << " |";
    }
    std::cout << std::endl;
    return initialBounds;
}

void TwoHalfD::BSPManager::_findSegmentIntersections(const TwoHalfD::XYVectorf &p1, const float radius, BSPNode *node,
                                                     std::vector<Segment> &intersectedSegments) {
    if (node == nullptr) return;

    if (node->segmentID != -1) {
        const auto &segment = m_segments[node->segmentID];
        auto intersections = circleLineIntersect(p1, radius, segment.v1, segment.v2);
        if (!intersections.empty()) {
            intersectedSegments.push_back(segment);
        }
    }

    float signedDist = crossProduct2d(p1 - node->splitterP0, node->splitterVec.normalized());

    if (signedDist > -radius) {
        _findSegmentIntersections(p1, radius, node->back.get(), intersectedSegments);
    }
    if (signedDist < radius) {
        _findSegmentIntersections(p1, radius, node->front.get(), intersectedSegments);
    }
}

TwoHalfD::BSPNode *TwoHalfD::BSPManager::_findConvexSection(const TwoHalfD::XYVectorf &point, BSPNode *node) {
    if (node == nullptr) return nullptr;

    if (node->front == nullptr && node->back == nullptr) {
        return node;
    }

    bool isInfrontOfSplit = isInfront(point - node->splitterP0, node->splitterVec);

    if (isInfrontOfSplit) {
        return _findConvexSection(point, node->front.get());
    } else {
        return _findConvexSection(point, node->back.get());
    }
}
