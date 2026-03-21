#include <SFML/Graphics.hpp>
#include <TwoHalfD/bsp/bsp_manager.h>
#include <TwoHalfD/level_maker.h>

#include <algorithm>
#include <filesystem>
#include <limits>
#include <queue>

namespace fs = std::filesystem;

// Runs A* and records the order nodes are expanded, plus the final waypoint path.
static void computePathVisualization(const TwoHalfD::BSPGraph &graph, const TwoHalfD::XYVectorf &start, const TwoHalfD::XYVectorf &end,
                                     std::vector<int> &exploredOut, std::vector<TwoHalfD::XYVectorf> &pathOut) {
    pathOut = graph.findPath(start, end, 0.f, 20.f, 0.f);

    int startNode = graph.findNodeForPoint(start);
    int endNode = graph.findNodeForPoint(end);
    if (startNode == -1 || endNode == -1) return;

    int n = graph.getNodeCount();
    std::vector<float> gScore(n, std::numeric_limits<float>::max());
    std::priority_queue<std::pair<float, int>, std::vector<std::pair<float, int>>, std::greater<>> openSet;

    gScore[startNode] = 0.f;
    openSet.push({(graph.getNode(startNode).centroid - graph.getNode(endNode).centroid).length(), startNode});

    while (!openSet.empty()) {
        auto [f, current] = openSet.top();
        openSet.pop();

        float h = (graph.getNode(current).centroid - graph.getNode(endNode).centroid).length();
        if (f > gScore[current] + h) continue; // stale

        exploredOut.push_back(current);
        if (current == endNode) break;

        for (const auto &edge : graph.getNode(current).edges) {
            float stepCost = (graph.getNode(current).centroid - edge.portalMidpoint).length() +
                             (edge.portalMidpoint - graph.getNode(edge.targetNodeIndex).centroid).length();
            float tentativeG = gScore[current] + stepCost;
            if (tentativeG >= gScore[edge.targetNodeIndex]) continue;
            gScore[edge.targetNodeIndex] = tentativeG;
            float newH = (graph.getNode(edge.targetNodeIndex).centroid - graph.getNode(endNode).centroid).length();
            openSet.push({tentativeG + newH, edge.targetNodeIndex});
        }
    }
}

int main() {
    TwoHalfD::LevelMaker levelMaker;
    fs::path levelFile = fs::path(ASSETS_DIR) / "levels" / "level1.txt";
    TwoHalfD::Level level = levelMaker.parseLevelFile(levelFile.string());

    TwoHalfD::BSPManager bspManager(&level);
    bspManager.buildBSPTree();
    bspManager.buildGraph();

    const TwoHalfD::BSPGraph &graph = bspManager.getGraph();

    // Compute world bounding box
    float minX = std::numeric_limits<float>::max(), minY = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest(), maxY = std::numeric_limits<float>::lowest();
    for (const auto &wall : level.walls) {
        minX = std::min({minX, wall.start.x, wall.end.x});
        minY = std::min({minY, wall.start.y, wall.end.y});
        maxX = std::max({maxX, wall.start.x, wall.end.x});
        maxY = std::max({maxY, wall.start.y, wall.end.y});
    }
    for (const auto &[id, fs] : level.floorSections) {
        for (const auto &v : fs.vertices) {
            minX = std::min(minX, v.x);
            minY = std::min(minY, v.y);
            maxX = std::max(maxX, v.x);
            maxY = std::max(maxY, v.y);
        }
    }

    // Pre-compute A* exploration steps and final path
    const TwoHalfD::XYVectorf pathStart{2800.f, 370.f};
    const TwoHalfD::XYVectorf pathEnd{300.f, 300.f};
    std::vector<int> exploredNodes;
    std::vector<TwoHalfD::XYVectorf> finalPath;
    computePathVisualization(graph, pathStart, pathEnd, exploredNodes, finalPath);

    const float WIN_W = 1280.f, WIN_H = 960.f;
    sf::RenderWindow window(sf::VideoMode(static_cast<unsigned>(WIN_W), static_cast<unsigned>(WIN_H)), "BSP Visualizer");
    window.setFramerateLimit(60);

    float worldW = maxX - minX, worldH = maxY - minY;
    float scale = std::min(WIN_W / (worldW * 1.1f), WIN_H / (worldH * 1.1f));
    sf::Vector2f viewCenter{(minX + maxX) / 2.f, (minY + maxY) / 2.f};

    auto toScreen = [&](const TwoHalfD::XYVectorf &p) -> sf::Vector2f {
        return {(p.x - viewCenter.x) * scale + WIN_W / 2.f, (p.y - viewCenter.y) * scale + WIN_H / 2.f};
    };

    auto drawLine = [&](const TwoHalfD::XYVectorf &a, const TwoHalfD::XYVectorf &b, sf::Color col) {
        sf::Vertex line[2] = {sf::Vertex(toScreen(a), col), sf::Vertex(toScreen(b), col)};
        window.draw(line, 2, sf::Lines);
    };

    auto drawDot = [&](const TwoHalfD::XYVectorf &p, float r, sf::Color col) {
        sf::CircleShape dot(r);
        dot.setFillColor(col);
        sf::Vector2f pos = toScreen(p);
        dot.setPosition(pos.x - r, pos.y - r);
        window.draw(dot);
    };

    bool panning = false;
    sf::Vector2i lastMouse;
    sf::Clock pathClock;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) window.close();
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::R) pathClock.restart();

            if (event.type == sf::Event::MouseWheelScrolled) scale *= (event.mouseWheelScroll.delta > 0) ? 1.15f : (1.f / 1.15f);

            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Middle) {
                panning = true;
                lastMouse = sf::Mouse::getPosition(window);
            }
            if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Middle) panning = false;

            if (event.type == sf::Event::MouseMoved && panning) {
                sf::Vector2i curr = sf::Mouse::getPosition(window);
                sf::Vector2i delta = curr - lastMouse;
                viewCenter.x -= delta.x / scale;
                viewCenter.y -= delta.y / scale;
                lastMouse = curr;
            }
        }

        // How many exploration steps to show (1 per second)
        int stepsToShow = static_cast<int>(pathClock.getElapsedTime().asSeconds());
        stepsToShow = std::min(stepsToShow, static_cast<int>(exploredNodes.size()));
        bool showFinalPath = stepsToShow >= static_cast<int>(exploredNodes.size());

        window.clear(sf::Color(18, 18, 18));

        // --- Dark grey: convex region outlines ---
        for (const auto &node : graph.getNodes()) {
            const auto &bounds = node.bspNode->bounds;
            int n = static_cast<int>(bounds.size());
            for (int i = 0; i < n; ++i)
                drawLine(bounds[i], bounds[(i + 1) % n], sf::Color(70, 70, 70));
        }

        // --- Blue: graph edges (centroid -> portal -> centroid) ---
        for (int i = 0; i < graph.getNodeCount(); ++i) {
            const auto &node = graph.getNode(i);
            for (const auto &edge : node.edges) {
                if (edge.targetNodeIndex <= i) continue;
                drawLine(node.centroid, edge.portalMidpoint, sf::Color(60, 140, 255));
                drawLine(edge.portalMidpoint, graph.getNode(edge.targetNodeIndex).centroid, sf::Color(60, 140, 255));
            }
        }

        // --- Orange dots: portal midpoints ---
        for (int i = 0; i < graph.getNodeCount(); ++i)
            for (const auto &edge : graph.getNode(i).edges)
                if (edge.targetNodeIndex > i) drawDot(edge.portalMidpoint, 3.f, sf::Color(255, 160, 0));

        // --- White dots: node centroids ---
        for (int i = 0; i < graph.getNodeCount(); ++i)
            drawDot(graph.getNode(i).centroid, 3.f, sf::Color(200, 200, 200));

        // --- Yellow: A* explored node centroids ---
        for (int step = 0; step < stepsToShow; ++step)
            drawDot(graph.getNode(exploredNodes[step]).centroid, 6.f, sf::Color(255, 220, 0));

        // --- White: floor section boundaries and walls (drawn on top) ---
        for (const auto &[id, floorSec] : level.floorSections) {
            int n = static_cast<int>(floorSec.vertices.size());
            for (int i = 0; i < n; ++i)
                drawLine(floorSec.vertices[i], floorSec.vertices[(i + 1) % n], sf::Color(220, 220, 220));
        }
        for (const auto &wall : level.walls)
            drawLine({wall.start.x, wall.start.y}, {wall.end.x, wall.end.y}, sf::Color::White);

        // --- Green: final path ---
        if (showFinalPath && finalPath.size() >= 2) {
            for (int i = 0; i < static_cast<int>(finalPath.size()) - 1; ++i)
                drawLine(finalPath[i], finalPath[i + 1], sf::Color(0, 255, 120));
            for (const auto &wp : finalPath)
                drawDot(wp, 5.f, sf::Color(0, 255, 120));
        }

        // --- Start (red) and end (cyan) markers ---
        drawDot(pathStart, 7.f, sf::Color(255, 50, 50));
        drawDot(pathEnd, 7.f, sf::Color(50, 200, 255));

        window.display();
    }

    return 0;
}
