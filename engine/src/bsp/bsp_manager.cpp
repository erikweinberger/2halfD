
#include "TwoHalfD/bsp/bsp_manager.h"
#include "../utils/math_util.h"
#include "TwoHalfD/engine_types.h"
#include <cstddef>
#include <iostream>
#include <limits>
#include <memory>
#include <queue>
#include <random>
#include <unordered_map>
#include <vector>

void TwoHalfD::BSPManager::buildBSPTree() {
    int seed = m_level->seed;
    std::cout << "Seed for BSP: " << seed << std::endl;
    if (seed == -1) {
        seed = findBestPartitioning();
    }

    m_root = std::make_unique<TwoHalfD::BSPNode>();
    std::vector<TwoHalfD::Segment> segments;
    segments.reserve(m_level->walls.size());
    for (const auto &wall : m_level->walls) {
        segments.push_back({{wall.start.x, wall.start.y}, {wall.end.x, wall.end.y}, &wall});
    }
    std::mt19937 rng(seed);
    std::shuffle(segments.begin(), segments.end(), rng);
    OptimalCostPartitioning cost{0, 0, 0};
    _buildBSPTree(m_root.get(), segments, cost);

    insertSprites(m_level->sprites);
    insertFloorSections(m_level->floorSections);

    return;
}

void TwoHalfD::BSPManager::insertSprites(const std::vector<SpriteEntity> &sprites) {
    for (size_t i{}; i < sprites.size(); ++i) {
        _insertSprite(m_root.get(), sprites[i], i);
    }
}

void TwoHalfD::BSPManager::insertFloorSections(const std::unordered_map<int, FloorSection> &floorSections) {
    for (const auto &floorSection : floorSections) {
        for (size_t j{}; j < floorSection.second.vertices.size(); ++j) {
            _insertFloorSection(m_root.get(), floorSection.second, j);
        }
    }
}

TwoHalfD::Segment &TwoHalfD::BSPManager::getSegment(int id) {
    return m_segments[id];
}

std::pair<std::vector<TwoHalfD::DrawCommand>, std::unordered_set<int>> TwoHalfD::BSPManager::update(TwoHalfD::Position &cameraPos) {
    TwoHalfD::XYVectorf cameraDir{std::cos(cameraPos.direction), std::sin(cameraPos.direction)};
    std::vector<TwoHalfD::DrawCommand> commands;
    std::unordered_set<int> floorSectionIds;
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

        for (const auto &spriteId : node->spriteIds) {
            TwoHalfD::XYVectorf spritePos = m_level->sprites[spriteId].pos.pos;
            float distance = (spritePos - cameraPos.pos).length();
            spriteOrderedDistance.push({distance, spriteId});
        }

        if (!isInfrontOfCamera) {
            while (!spriteOrderedDistance.empty()) {
                commands.push_back({TwoHalfD::DrawCommand::Type::Sprite, spriteOrderedDistance.top().second});
                spriteOrderedDistance.pop();
            }
            commands.push_back({TwoHalfD::DrawCommand::Type::Segment, static_cast<int>(node->segmentID)});
        } else {
            commands.push_back({TwoHalfD::DrawCommand::Type::Segment, static_cast<int>(node->segmentID)});
            while (!spriteOrderedDistance.empty()) {
                commands.push_back({TwoHalfD::DrawCommand::Type::Sprite, spriteOrderedDistance.top().second});
                spriteOrderedDistance.pop();
            }
        }
        return;
    }

    if (isInfrontOfCamera) {
        traverse(node->back.get(), commands, floorSectionIds, cameraPos);

        commands.push_back({TwoHalfD::DrawCommand::Type::Segment, node->segmentID});
        traverse(node->front.get(), commands, floorSectionIds, cameraPos);

    } else {
        traverse(node->front.get(), commands, floorSectionIds, cameraPos);

        commands.push_back({TwoHalfD::DrawCommand::Type::Segment, node->segmentID});

        traverse(node->back.get(), commands, floorSectionIds, cameraPos);
    }
}

void TwoHalfD::BSPManager::traverse(TwoHalfD::BSPNode *node, std::vector<TwoHalfD::DrawCommand> &commands, std::unordered_set<int> &floorSectionIds,
                                    const TwoHalfD::Position &cameraPos, const TwoHalfD::XYVectorf &cameraDir) {
    if (node == nullptr) return;

    bool isInfrontOfCamera = isInfront(cameraPos.pos - node->splitterP0, node->splitterVec);

    if (node->front == nullptr && node->back == nullptr) {

        auto cmp = [](const auto &a, const auto &b) { return a.first > b.first; };
        std::priority_queue<std::pair<float, int>, std::vector<std::pair<float, int>>, decltype(cmp)> spriteOrderedDistance(cmp);

        for (const auto &spriteId : node->spriteIds) {
            TwoHalfD::XYVectorf spritePos = m_level->sprites[spriteId].pos.pos;
            float distance = (spritePos - cameraPos.pos).length();
            spriteOrderedDistance.push({distance, spriteId});
        }

        if (!isInfrontOfCamera) {
            while (!spriteOrderedDistance.empty()) {
                commands.push_back({TwoHalfD::DrawCommand::Type::Sprite, spriteOrderedDistance.top().second});
                spriteOrderedDistance.pop();
            }
            commands.push_back({TwoHalfD::DrawCommand::Type::Segment, static_cast<int>(node->segmentID)});
        } else {
            commands.push_back({TwoHalfD::DrawCommand::Type::Segment, static_cast<int>(node->segmentID)});
            while (!spriteOrderedDistance.empty()) {
                commands.push_back({TwoHalfD::DrawCommand::Type::Sprite, spriteOrderedDistance.top().second});
                spriteOrderedDistance.pop();
            }
        }
        for (const auto &floorSectionId : node->floorSectionIds) {
            floorSectionIds.insert(floorSectionId);
        }
        return;
    }

    if (isInfrontOfCamera) {
        traverse(node->back.get(), commands, floorSectionIds, cameraPos, cameraDir);

        commands.push_back({TwoHalfD::DrawCommand::Type::Segment, node->segmentID});
        traverse(node->front.get(), commands, floorSectionIds, cameraPos, cameraDir);

    } else {
        traverse(node->front.get(), commands, floorSectionIds, cameraPos, cameraDir);

        commands.push_back({TwoHalfD::DrawCommand::Type::Segment, node->segmentID});

        traverse(node->back.get(), commands, floorSectionIds, cameraPos, cameraDir);
    }
}

// Getters and setters
void TwoHalfD::BSPManager::setLevel(const TwoHalfD::Level *level) {
    m_level = level;
}

int TwoHalfD::BSPManager::findBestPartitioning() {
    std::vector<TwoHalfD::Segment> segments;
    segments.reserve(m_level->walls.size());
    for (const auto &wall : m_level->walls) {
        segments.push_back({{wall.start.x, wall.start.y}, {wall.end.x, wall.end.y}, &wall});
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
    _buildBSPTree(&rootNode, segments, cost, false);

    float score = std::abs(cost.numBack - cost.numFront) + (cost.splitCount * m_splitWeight);
    return score;
}

/* =============================================================================================================================
 * Private functions
 * =============================================================================================================================
 */
void TwoHalfD::BSPManager::_buildBSPTree(TwoHalfD::BSPNode *node, const std::vector<TwoHalfD::Segment> &inputSegments,
                                         struct OptimalCostPartitioning &cost, bool saveSegments) {
    if (inputSegments.size() == 0) {
        return;
    }

    auto [frontSegs, backSegs] = _splitSpace(node, inputSegments, cost, saveSegments);

    if (backSegs.size() > 0) {
        node->back = std::make_unique<TwoHalfD::BSPNode>();
        cost.numBack += 1;
        _buildBSPTree(node->back.get(), backSegs, cost, saveSegments);
    }

    if (frontSegs.size() > 0) {
        node->front = std::make_unique<TwoHalfD::BSPNode>();
        cost.numFront += 1;
        _buildBSPTree(node->front.get(), frontSegs, cost, saveSegments);
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
                cost.splitCount += 1;
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

    if (saveSegments) {
        _addSegment(std::move(splitterSeg), node);
    }
    return {frontSegs, backSegs};
}

void TwoHalfD::BSPManager::_insertSprite(TwoHalfD::BSPNode *node, const SpriteEntity &sprite, int spriteId) {
    if (node == nullptr) return;

    // If this is a leaf node, add the sprite here
    if (node->front == nullptr && node->back == nullptr) {
        node->spriteIds.insert(spriteId);
        return;
    }

    float isInfrontOfSplit = isInfront(sprite.pos.pos - node->splitterP0, node->splitterVec);

    if (isInfrontOfSplit > std::numeric_limits<float>::epsilon()) {
        _insertSprite(node->front.get(), sprite, spriteId);
    } else {
        _insertSprite(node->back.get(), sprite, spriteId);
    }
}

void TwoHalfD::BSPManager::_insertFloorSection(TwoHalfD::BSPNode *node, const FloorSection &floorSection, int vertexId) {
    if (node == nullptr) return;

    // If this is a leaf node, add the floor section here
    if (node->front == nullptr && node->back == nullptr) {
        node->floorSectionIds.insert(floorSection.id);
        return;
    }

    float isInfrontOfSplit = isInfront(floorSection.vertices[vertexId] - node->splitterP0, node->splitterVec);

    if (isInfrontOfSplit > std::numeric_limits<float>::epsilon()) {
        _insertFloorSection(node->front.get(), floorSection, vertexId);
    } else {
        _insertFloorSection(node->back.get(), floorSection, vertexId);
    }
}

void TwoHalfD::BSPManager::_addSegment(TwoHalfD::Segment &&segment, TwoHalfD::BSPNode *node) {
    m_segments.push_back(std::move(segment));
    node->segmentID = m_segmentID;

    ++m_segmentID;
}
