// #include "TwoHalfD/engine_types.h"
// #include "game.h"
// #include <SFML/Graphics.hpp>
// #include <SFML/Graphics/PrimitiveType.hpp>
// #include <TwoHalfD/bsp/bsp_manager.h>
// #include <TwoHalfD/engine.h>
// #include <TwoHalfD/level_maker.h>
// #include <chrono>
// #include <iostream>
// #include <thread>

// int main() {
//     std::cerr << "Starting\n";

//     // Load level and build BSP tree
//     TwoHalfD::LevelMaker levelMaker;
//     fs::path levelFile = fs::path(ASSETS_DIR) / "levels" / "level_test_bsp.txt";
//     auto level = levelMaker.parseLevelFile(levelFile);

//     TwoHalfD::BSPManager bspManager(level);
//     bspManager.buildBSPTree();

//     // Test position
//     TwoHalfD::Position pos{600, 700, 2.356f};

//     // Create window
//     sf::RenderWindow window(sf::VideoMode(1200, 1000), "BSP Tree Test");

//     // Get visible segments
//     auto segments = bspManager.update(pos);

//     std::cerr << "BSP tree built. Visible segments: " << segments.size() << "\n";
//     std::cerr << "Total walls: " << level.walls.size() << "\n";

//     // Draw player position
//     sf::CircleShape playerDot(5.f);
//     playerDot.setFillColor(sf::Color::Red);
//     playerDot.setPosition(pos.pos.x - 5.f, pos.pos.y - 5.f);

//     TwoHalfD::XYVectorf cameraDir{std::cos(pos.direction), std::sin(pos.direction)};
//     sf::Vertex lineCameraDir[] = {
//         sf::Vertex(sf::Vector2f(pos.pos.x - 5.f, pos.pos.y - 5.f), sf::Color::Yellow),
//         sf::Vertex(sf::Vector2f(pos.pos.x - 5.f + cameraDir.x * 20, pos.pos.y - 5.f + cameraDir.y * 20), sf::Color::Yellow)};

//     // Track how many segments to draw
//     size_t segmentsToShow = 0;
//     sf::Clock clock;
//     const float delayBetweenSegments = 0.5f; // 500ms in seconds

//     // Main loop
//     while (window.isOpen()) {
//         sf::Event event;
//         while (window.pollEvent(event)) {
//             if (event.type == sf::Event::Closed) {
//                 window.close();
//             }
//             if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
//                 window.close();
//             }
//         }

//         // Add new segment every 500ms
//         if (clock.getElapsedTime().asSeconds() >= delayBetweenSegments && segmentsToShow < segments.size()) {
//             segmentsToShow++;
//             std::cout << "Showing segment " << segmentsToShow << " of " << segments.size() << "\n";
//             clock.restart();
//         }

//         // Clear and redraw everything each frame
//         window.clear(sf::Color::Black);

//         // Draw ALL level walls in gray (for reference)
//         for (const auto &wall : level.walls) {
//             sf::Vertex line[] = {sf::Vertex(sf::Vector2f(wall.start.x, wall.start.y), sf::Color(80, 80, 80)),
//                                  sf::Vertex(sf::Vector2f(wall.end.x, wall.end.y), sf::Color(80, 80, 80))};
//             window.draw(line, 2, sf::Lines);
//         }

//         // Draw player position
//         window.draw(playerDot);
//         window.draw(lineCameraDir, 2, sf::Lines);

//         // Draw BSP visible segments in yellow (appearing one by one)
//         for (size_t i = 0; i < segmentsToShow; ++i) {
//             auto segment = bspManager.getSegment(segments[i]);

//             sf::Vertex line[] = {sf::Vertex(sf::Vector2f(segment.v1.x, segment.v1.y), sf::Color::Yellow),
//                                  sf::Vertex(sf::Vector2f(segment.v2.x, segment.v2.y), sf::Color::Yellow)};

//             window.draw(line, 2, sf::Lines);
//         }

//         window.display();

//         // Close after showing all segments + 5 second delay
//         if (segmentsToShow >= segments.size()) {
//             static sf::Clock closeClock;
//             static bool startedCloseClock = false;

//             if (!startedCloseClock) {
//                 closeClock.restart();
//                 startedCloseClock = true;
//             }

//             if (closeClock.getElapsedTime().asSeconds() >= 5.0f) {
//                 window.close();
//             }
//         }
//     }

//     return 0;
// }