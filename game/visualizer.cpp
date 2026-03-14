#include <SFML/Graphics.hpp>
#include <TwoHalfD/bsp/bsp_manager.h>
#include <TwoHalfD/level_maker.h>

#include <filesystem>
#include <limits>

namespace fs = std::filesystem;

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

    const float WIN_W = 1280.f, WIN_H = 960.f;
    sf::RenderWindow window(sf::VideoMode(static_cast<unsigned>(WIN_W), static_cast<unsigned>(WIN_H)), "BSP Visualizer");
    window.setFramerateLimit(60);

    // Fit level into window with 5% padding on each side
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

    // Pan/zoom
    bool panning = false;
    sf::Vector2i lastMouse;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) window.close();

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

        window.clear(sf::Color(18, 18, 18));

        // --- Dark grey: convex region outlines (the BSP cell boundaries = actual segments + their extensions) ---
        for (const auto &node : graph.getNodes()) {
            const auto &bounds = node.bspNode->bounds;
            int n = static_cast<int>(bounds.size());
            for (int i = 0; i < n; ++i)
                drawLine(bounds[i], bounds[(i + 1) % n], sf::Color(70, 70, 70));
        }

        // --- White: floor section boundaries ---
        for (const auto &[id, floorSec] : level.floorSections) {
            int n = static_cast<int>(floorSec.vertices.size());
            for (int i = 0; i < n; ++i)
                drawLine(floorSec.vertices[i], floorSec.vertices[(i + 1) % n], sf::Color(220, 220, 220));
        }

        // --- White: walls (drawn on top so they're clearly visible) ---
        for (const auto &wall : level.walls)
            drawLine({wall.start.x, wall.start.y}, {wall.end.x, wall.end.y}, sf::Color::White);

        // --- Graph edges drawn as centroid → portal midpoint → centroid ---
        // This shows the actual safe waypoint path. Blue = bidirectional, orange = drop-only.
        for (int i = 0; i < graph.getNodeCount(); ++i) {
            const auto &node = graph.getNode(i);
            for (const auto &edge : node.edges) {
                if (edge.isDropOnly || edge.targetNodeIndex > i) { // avoid drawing bidirectional edges twice
                    sf::Color col = edge.isDropOnly ? sf::Color(255, 130, 0) : sf::Color(60, 140, 255);
                    TwoHalfD::XYVectorf mid = edge.portalMidpoint();
                    drawLine(node.centroid, mid, col);
                    drawLine(mid, graph.getNode(edge.targetNodeIndex).centroid, col);
                }
            }
        }

        // --- Portal midpoints (green dots) ---
        for (int i = 0; i < graph.getNodeCount(); ++i) {
            for (const auto &edge : graph.getNode(i).edges) {
                if (edge.isDropOnly || edge.targetNodeIndex > i) {
                    sf::Vector2f pos = toScreen(edge.portalMidpoint());
                    sf::CircleShape dot(3.f);
                    dot.setFillColor(sf::Color(0, 220, 100));
                    dot.setPosition(pos.x - 3.f, pos.y - 3.f);
                    window.draw(dot);
                }
            }
        }

        // --- Yellow dots: node centroids ---
        for (int i = 0; i < graph.getNodeCount(); ++i) {
            sf::Vector2f pos = toScreen(graph.getNode(i).centroid);
            sf::CircleShape dot(3.f);
            dot.setFillColor(sf::Color(255, 220, 0));
            dot.setPosition(pos.x - 3.f, pos.y - 3.f);
            window.draw(dot);
        }

        window.display();
    }

    return 0;
}
