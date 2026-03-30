#ifndef BSP_MANAGER_H
#define BSP_MANAGER_H

#include "TwoHalfD/bsp/bsp_graph.h"
#include "TwoHalfD/engine_types.h"
#include <cstddef>
#include <memory>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace TwoHalfD {

struct OptimalCostPartitioning {
    int splitCount;
    int numFront;
    int numBack;
};

static constexpr float BSP_EPSILON = 0.01f;

class BSPManager {
  public:
    BSPManager() = default;

    // Initialization — takes ownership of level geometry
    void init(std::vector<Wall> walls, std::unordered_map<int, FloorSection> floorSections, float defaultFloorHeight, int defaultFloorTextureId,
              int seed);

    // Construction
    void buildBSPTree();
    void buildGraph();
    std::unordered_map<int, float> insertSprites(const std::unordered_map<int, SpriteEntity> &entities);
    float moveSprite(int entityId, TwoHalfD::XYVectorf newPos);

    // Core functions
    TwoHalfD::BSPNode *findConvexSection(const TwoHalfD::XYVectorf &point);

    // Traverse logic
    std::pair<std::vector<TwoHalfD::DrawCommand>, std::unordered_set<int>> update(TwoHalfD::Position &cameraPos);
    void traverse(TwoHalfD::BSPNode *node, std::vector<TwoHalfD::DrawCommand> &commands, std::unordered_set<int> &floorSectionIds,
                  const TwoHalfD::Position &cameraPos);
    void traverse(TwoHalfD::BSPNode *node, std::vector<TwoHalfD::DrawCommand> &commands, std::unordered_set<int> &floorSectionIds,
                  const TwoHalfD::Position &cameraPos, const TwoHalfD::XYVectorf &cameraDir);

    // Getters
    TwoHalfD::Segment &getSegment(int id);
    TwoHalfD::BSPGraph &getGraph();
    const std::vector<Wall> &getWalls() const;
    const std::unordered_map<int, FloorSection> &getFloorSections() const;

    // BSP optimization
    int findBestPartitioning();

    // Collision
    std::vector<TwoHalfD::Segment> findSegmentIntersection(const TwoHalfD::XYVectorf &p1, const float radius);
    std::vector<std::pair<TwoHalfD::XYVectorf, TwoHalfD::XYVectorf>> findCollisions(const TwoHalfD::XYVectorf &pos, float radius,
                                                                                    float cameraHeightStart);

    TwoHalfD::Path findPath(const TwoHalfD::XYVectorf &start, const TwoHalfD::XYVectorf &end, float entityWidth, float maxHeightDiff,
                            float maxDistance);

  private:
    // Owned level geometry
    std::vector<Wall> m_walls;
    std::unordered_map<int, FloorSection> m_floorSections;
    float m_defaultFloorHeight = 0.f;
    int m_defaultFloorTextureId = -1;
    int m_seed = -1;

    std::unique_ptr<TwoHalfD::BSPNode> m_root;
    std::unordered_map<int, TwoHalfD::BSPNode *> m_spriteNodeMap; // entityId → BSP node
    std::unordered_map<int, XYVectorf> m_spritePositions;         // entityId → position (for traverse)
    TwoHalfD::BSPGraph m_graph;
    std::vector<TwoHalfD::Segment> m_segments;
    size_t m_segmentID = 0;
    float m_splitWeight = 3.f;
    int m_startSeed = 0;
    int m_endSeed = 20000;

    void _buildBSPTree(TwoHalfD::BSPNode *node, const std::vector<TwoHalfD::Segment> &inputSegments, Polygon bounds, int floorSectionId,
                       struct OptimalCostPartitioning &cost, bool saveSegments = true);
    std::pair<std::vector<TwoHalfD::Segment>, std::vector<TwoHalfD::Segment>> _splitSpace(TwoHalfD::BSPNode *node,
                                                                                          const std::vector<TwoHalfD::Segment> &inputSegments,
                                                                                          struct OptimalCostPartitioning &cost,
                                                                                          bool saveSegments = true);

    // Construction
    void _addSegment(TwoHalfD::Segment &&segment, TwoHalfD::BSPNode *node);
    float _insertSprite(TwoHalfD::BSPNode *node, int entityId, TwoHalfD::XYVectorf pos);

    // Core functionality
    TwoHalfD::BSPNode *_findConvexSection(const TwoHalfD::XYVectorf &point, TwoHalfD::BSPNode *node);

    // BSP optimization
    float _findIndividualPartitioning(int seed, std::vector<TwoHalfD::Segment> segments);

    // Find bounding box of a set of segments
    std::pair<Polygon, Polygon> _splitConvexShape(const Polygon &vertices, const TwoHalfD::Segment &splitter);
    TwoHalfD::Polygon _getInitialBounds(const std::vector<TwoHalfD::Segment> &segments);

    // Collision
    void _findSegmentIntersections(const TwoHalfD::XYVectorf &p1, float radius, TwoHalfD::BSPNode *node,
                                   std::vector<TwoHalfD::Segment> &intersectedSegments);

    // Path smoothing
    TwoHalfD::Path _smoothPath(const TwoHalfD::Path &path, float entityWidth, float maxHeightDiff);
    bool _hasLineOfSight(const TwoHalfD::XYVectorf &a, const TwoHalfD::XYVectorf &b, float entityWidth, float maxHeightDiff);
};
} // namespace TwoHalfD

#endif