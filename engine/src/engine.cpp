#include "TwoHalfD/bsp/bsp_manager.h"
#include "TwoHalfD/engine_types.h"
#include "utils/math_util.h"

#include <SFML/Graphics/BlendMode.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/PrimitiveType.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Vertex.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Mouse.hpp>
#include <TwoHalfD/engine.h>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <limits>
#include <queue>
#include <thread>

void TwoHalfD::Engine::loadLevel(std::string levelFilePath) {
    this->m_engineState = EngineState::fpsState;
    m_window.setMouseCursorVisible(false);
    m_level = m_levelMaker.parseLevelFile(levelFilePath);
    m_bspManager.setLevel(&m_level);
    m_bspManager.buildBSPTree();
}

// Game Inputs
std::span<const TwoHalfD::Event> TwoHalfD::Engine::getFrameInputs() {
    sf::Event event;
    while (m_window.pollEvent(event)) {
        switch (event.type) {
        case sf::Event::Closed: {
            m_window.close();
            m_engineState = EngineState::ended;
            break;
        }
        case sf::Event::KeyPressed: {
            sf::Vector2i mouseWinPos = sf::Mouse::getPosition(m_window);
            m_inputArray[m_currentInput] = TwoHalfD::Event::KeyPressed(event.key.code, mouseWinPos.x, mouseWinPos.y);
            ++m_currentInput;
            break;
        }
        case sf::Event::KeyReleased: {
            sf::Vector2i mouseWinPos = sf::Mouse::getPosition(m_window);
            m_inputArray[m_currentInput] = TwoHalfD::Event::KeyReleased(event.key.code, mouseWinPos.x, mouseWinPos.y);
            ++m_currentInput;
            break;
        }
        case sf::Event::MouseMoved: {
            // Get position relative to window, not global screen
            XYVector mouseWinPos = {event.mouseMove.x, event.mouseMove.y};
            auto size = m_window.getSize();
            static XYVector middleScreen = {(int)size.x / 2, (int)size.y / 2};

            m_engineContext.MouseDelta = m_engineContext.prevMousePosition - mouseWinPos;
            m_engineContext.prevMousePosition = m_engineContext.currentMousePosition;
            m_engineContext.currentMousePosition = {event.mouseMove.x, event.mouseMove.y};
            if (std::abs(m_engineContext.MouseDelta.x) > 0.8 * middleScreen.x || std::abs(m_engineContext.MouseDelta.y) > 0.8 * middleScreen.y) {
                m_engineContext.prevMousePosition = m_engineContext.currentMousePosition;
                break;
            }

            m_inputArray[m_currentInput] = TwoHalfD::Event::MouseMoved(event.mouseMove.x, event.mouseMove.y, m_engineContext.MouseDelta);
            ++m_currentInput;
            break;
        }
        default:
            break;
        }
    }
    backgroundFrameUpdates();
    return std::span<const TwoHalfD::Event>(m_inputArray.data(), m_currentInput);
}

void TwoHalfD::Engine::clearFrameInputs() {
    m_currentInput = 0;
}

void TwoHalfD::Engine::backgroundFrameUpdates() {
    if (m_engineState == TwoHalfD::EngineState::fpsState) {
        auto size = m_window.getSize();
        static XYVector middleScreen = {(int)size.x / 2, (int)size.y / 2};
        sf::Vector2i mousePosition = sf::Mouse::getPosition(m_window);

        if (m_window.hasFocus() && (m_engineSettings.windowDim.x - mousePosition.x < 0 || mousePosition.x < 0 ||
                                    m_engineSettings.windowDim.y - mousePosition.y < 0 || mousePosition.y < 0)) {
            sf::Mouse::setPosition({middleScreen.x, middleScreen.y}, m_window);
        }
    }
}

bool TwoHalfD::Engine::gameDeltaTimePassed() {
    return m_engineClocks.gameTimeDeltaPassed();
}

// Getters and setters
TwoHalfD::XYVector TwoHalfD::Engine::getMouseDeltaFrame() {
    return m_engineContext.MouseDelta;
}

TwoHalfD::XYVector TwoHalfD::Engine::getWindowDimension() {
    return {m_engineSettings.windowDim.x, m_engineSettings.windowDim.y};
}

TwoHalfD::Position TwoHalfD::Engine::getCameraPosition() {
    return m_cameraObject.cameraPos;
}

void TwoHalfD::Engine::setCameraPosition(const TwoHalfD::Position &newPos) {
    m_cameraObject.cameraPos = newPos;
}

TwoHalfD::Position TwoHalfD::Engine::updateCameraPosition(const TwoHalfD::Position &posUpdate) {
    TwoHalfD::Position prevPos = m_cameraObject.cameraPos;
    m_cameraObject.cameraPos += posUpdate;
    TwoHalfD::XYVectorf moveVec{posUpdate.pos.x, posUpdate.pos.y};
    TwoHalfD::XYVectorf n_moveVec{moveVec.normalized()};

    float moveMagnitude = 0;
    if (m_engineSettings.cameraCollision) {
        TwoHalfD::XYVectorf oldPos = prevPos.pos;
        auto wallReferences = wallCollisionSelf();
        if (wallReferences.size() > 0) {
            for (auto wall : wallReferences) {
                const TwoHalfD::XYVectorf wallVec = {(wall->end.x - wall->start.x), (wall->end.y - wall->start.y)};
                TwoHalfD::XYVectorf n_wallVec{wallVec.normalized()};

                const float perpPointDistToStart = TwoHalfD::dot(m_cameraObject.cameraPos.pos - wall->start, n_wallVec);
                TwoHalfD::XYVectorf perpP{wall->start + n_wallVec * perpPointDistToStart};

                TwoHalfD::XYVectorf perpVec{m_cameraObject.cameraPos.pos - perpP};
                TwoHalfD::XYVectorf n_perpVec{perpVec.normalized()};

                TwoHalfD::XYVectorf oldPerpVec{oldPos - perpP};
                float perpVecLen = perpVec.length();

                float penetrationDepth;
                if (dot(oldPerpVec, n_perpVec) < 0 && oldPerpVec.length() > perpVecLen) {
                    penetrationDepth = m_cameraObject.cameraRadius + perpVecLen;
                } else {
                    penetrationDepth = m_cameraObject.cameraRadius - perpVecLen;
                }
                float moveRatio = std::abs(dot(n_perpVec, n_moveVec));
                if (std::abs(moveRatio) < 0.01f) continue;

                moveMagnitude = std::min(std::max(moveMagnitude, penetrationDepth / moveRatio), moveVec.length());
            }

            TwoHalfD::Position newPos{m_cameraObject.cameraPos.pos - moveMagnitude * n_moveVec, m_cameraObject.cameraPos.direction};
            setCameraPosition(newPos);
            return m_cameraObject.cameraPos;
        }
    }

    return m_cameraObject.cameraPos;
}

void TwoHalfD::Engine::setState(TwoHalfD::EngineState newState) {
    m_engineState = newState;
}

TwoHalfD::EngineState TwoHalfD::Engine::getState() {
    return this->m_engineState;
}

std::vector<TwoHalfD::SpriteEntity> &TwoHalfD::Engine::getSpriteEntitiesInRegion() {
    return m_level.sprites;
}
std::vector<TwoHalfD::SpriteEntity> &TwoHalfD::Engine::getAllSpriteEntities() {
    return m_level.sprites;
}
std::vector<TwoHalfD::Wall> &TwoHalfD::Engine::getWallsInRegion() {
    return m_level.walls;
}
std::vector<TwoHalfD::Wall> &TwoHalfD::Engine::getAllWalls() {
    return m_level.walls;
}

// void TwoHalfD::Engine::renderAbove() {
//     m_window_above.clear(sf::Color::Black);

//     for (int i = 0; i < 1920; i += 256) {
//         sf::VertexArray lines(sf::LinesStrip, 2);
//         lines[0].position = sf::Vector2f(i, 0);
//         lines[1].position = sf::Vector2f(i, 1080);
//         lines[0].color = sf::Color::Yellow;
//         lines[1].color = sf::Color::Yellow;
//         m_window_above.draw(lines);
//     }
//     for (int i = 0; i < 1080; i += 256) {
//         sf::VertexArray lines(sf::LinesStrip, 2);
//         lines[0].position = sf::Vector2f(0, i);
//         lines[1].position = sf::Vector2f(1920, i);
//         lines[0].color = sf::Color::Yellow;
//         lines[1].color = sf::Color::Yellow;
//         m_window_above.draw(lines);
//     }

//     for (auto wall : m_level.walls) {
//         sf::VertexArray lines(sf::LinesStrip, 2);
//         lines[0].position = sf::Vector2f(wall.start.x, wall.start.y);
//         lines[1].position = sf::Vector2f(wall.end.x, wall.end.y);
//         lines[0].color = sf::Color::Red;
//         lines[1].color = sf::Color::Red;
//         m_window_above.draw(lines);
//     }
//     TwoHalfD::Position camP = getCameraPosition();
//     sf::CircleShape circle(10, 10);
//     circle.setFillColor(sf::Color::White);
//     circle.setPosition({100, 100});
//     circle.setPosition(camP.pos.x - 10, camP.pos.y - 10);
//     m_window_above.draw(circle);

//     sf::VertexArray lines(sf::LinesStrip, 2);
//     lines[0].position = sf::Vector2f(camP.pos.x, camP.pos.y);
//     lines[1].position = sf::Vector2f(camP.pos.x + 20 * std::cos(camP.direction), camP.pos.y + 20 * std::sin(camP.direction));
//     lines[0].color = sf::Color::Red;
//     lines[1].color = sf::Color::Red;
//     m_window_above.draw(lines);

//     m_window_above.display();
// }

// Render Logic
void TwoHalfD::Engine::render() {
    if (!m_engineClocks.graphicsTimeDeltaPassed()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        return;
    }
    m_renderTexture.clear(sf::Color::Transparent);
    renderFloor();
    renderWalls();
    // renderObjects();
    renderOverlays();
    // // renderAbove();

    m_renderTexture.display();
    sf::Sprite sprite(m_renderTexture.getTexture());
    sprite.setScale(static_cast<float>(m_engineSettings.windowDim.x) / m_engineSettings.resolution.x,
                    static_cast<float>(m_engineSettings.windowDim.y) / m_engineSettings.resolution.y);
    m_window.clear(sf::Color::Black);
    m_window.draw(sprite);
    m_window.display();
}

void TwoHalfD::Engine::renderOverlays() {
    static bool loaded = false;
    static sf::Font font;
    if (!loaded) {
        font.loadFromFile(fs::path(ASSETS_DIR) / "fonts" / "RasterForgeRegular-JpBgm.ttf");
        loaded = true;
    }

    sf::Text text;
    text.setFont(font);
    std::string fpsString = "Fps: " + std::to_string(static_cast<int>(std::round(m_engineClocks.getAverageGraphicsFps())));
    text.setString(fpsString);
    text.setCharacterSize(24);
    text.setFillColor(sf::Color::Yellow);
    text.setPosition(m_engineSettings.resolution.x - 120, m_engineSettings.resolution.y - 50);
    m_renderTexture.draw(text);

    sf::Text text1;
    text1.setFont(font);
    std::string position = "(" + std::to_string(m_cameraObject.cameraPos.posf.x) + ", " + std::to_string(m_cameraObject.cameraPos.posf.y) + ")";
    text1.setString(position);
    text1.setCharacterSize(24);
    text1.setFillColor(sf::Color::Yellow);
    text1.setPosition(50, m_engineSettings.resolution.y - 50);
    m_renderTexture.draw(text1);
}

void TwoHalfD::Engine::renderSegment(TwoHalfD::Segment segment) {
    const float NEAR_CLIP = 50.0f;

    const auto wall = segment.wall;
    float p_focalLength = (m_engineSettings.resolution.x / 2.0f) / m_engineSettings.fovScale;
    TwoHalfD::XYVectorf n_direction{std::cos(m_cameraObject.cameraPos.direction), std::sin(m_cameraObject.cameraPos.direction)};
    TwoHalfD::XYVectorf n_plane{-n_direction.y, n_direction.x};

    TwoHalfD::XYVectorf vecCamV1 = segment.v1 - m_cameraObject.cameraPos.pos;
    TwoHalfD::XYVectorf vecCamV2 = segment.v2 - m_cameraObject.cameraPos.pos;

    float wallRatioStart = segment.wallRatioStart;
    float wallRatioEnd = segment.wallRatioEnd;

    float singedPerpWorldDistanceStart = dotProduct(vecCamV1, n_direction);
    float singedPerpWorldDistanceEnd = dotProduct(vecCamV2, n_direction);

    if (singedPerpWorldDistanceEnd < NEAR_CLIP && singedPerpWorldDistanceStart < NEAR_CLIP) {
        return;
    }

    // Negative means left and positive means right
    float signedLateralDistV1 = dotProduct(vecCamV1, n_plane);
    float signedLateralDistV2 = dotProduct(vecCamV2, n_plane);

    bool v1RightV2Left = signedLateralDistV1 >= 0 && signedLateralDistV2 < 0; // v1 is right and v2 is left
    bool v2MoreLeft = std::signbit(signedLateralDistV1) == std::signbit(signedLateralDistV2) && signedLateralDistV1 > signedLateralDistV2;

    if (v1RightV2Left || v2MoreLeft) {
        std::swap(singedPerpWorldDistanceStart, singedPerpWorldDistanceEnd);
        std::swap(vecCamV1, vecCamV2);
        std::swap(signedLateralDistV1, signedLateralDistV2);
        std::swap(wallRatioStart, wallRatioEnd);
    }

    const float &halfYRes = m_engineSettings.resolution.y / 2.f;
    const float &halfXRes = m_engineSettings.resolution.x / 2.f;

    if (singedPerpWorldDistanceStart < NEAR_CLIP) {
        float t = (NEAR_CLIP - singedPerpWorldDistanceStart) / (singedPerpWorldDistanceEnd - singedPerpWorldDistanceStart);
        vecCamV1 = vecCamV1 + t * (vecCamV2 - vecCamV1);
        signedLateralDistV1 = dotProduct(vecCamV1, n_plane);
        singedPerpWorldDistanceStart = NEAR_CLIP;
        wallRatioStart = wallRatioStart + t * (wallRatioEnd - wallRatioStart);
    }
    if (singedPerpWorldDistanceEnd < NEAR_CLIP) {
        float t = (NEAR_CLIP - singedPerpWorldDistanceEnd) / (singedPerpWorldDistanceStart - singedPerpWorldDistanceEnd);
        vecCamV2 = vecCamV2 + t * (vecCamV1 - vecCamV2);
        signedLateralDistV2 = dotProduct(vecCamV2, n_plane);

        singedPerpWorldDistanceEnd = NEAR_CLIP;
        wallRatioEnd = wallRatioEnd + t * (wallRatioStart - wallRatioEnd);
    }

    float p_xScreenPosV1 = halfXRes + p_focalLength * signedLateralDistV1 / singedPerpWorldDistanceStart;
    float p_xScreenPosV2 = halfXRes + p_focalLength * signedLateralDistV2 / singedPerpWorldDistanceEnd;

    if ((p_xScreenPosV1 > m_engineSettings.resolution.x && p_xScreenPosV2 > m_engineSettings.resolution.x) ||
        (p_xScreenPosV2 < 0 && p_xScreenPosV1 < 0)) {
        return;
    }

    float p_topWallStart = p_focalLength * (m_cameraObject.cameraHeight - wall->height) / singedPerpWorldDistanceStart + halfYRes;
    float p_bottomWallStart = p_focalLength * (m_cameraObject.cameraHeight) / singedPerpWorldDistanceStart + halfYRes;

    float p_topWallEnd = p_focalLength * (m_cameraObject.cameraHeight - wall->height) / singedPerpWorldDistanceEnd + halfYRes;
    float p_bottomWallEnd = p_focalLength * (m_cameraObject.cameraHeight) / singedPerpWorldDistanceEnd + halfYRes;

    auto it = m_level.textures.find(wall->textureId);
    if (it == m_level.textures.end()) {
        std::cerr << "No texture found for wall: " << wall->id << " with texture id: " << wall->textureId << std::endl;
        exit(1);
    }

    sf::Texture &tex = it->second.texture;
    sf::Vector2u texSize = tex.getSize();

    const float wallLen = distanceBetweenPoints(wall->start, wall->end);

    sf::VertexArray quad(sf::Quads, 4);

    quad[0].position = sf::Vector2f(p_xScreenPosV1, p_topWallStart);

    quad[1].position = sf::Vector2f(p_xScreenPosV1, p_bottomWallStart);

    quad[2].position = sf::Vector2f(p_xScreenPosV2, p_bottomWallEnd);

    quad[3].position = sf::Vector2f(p_xScreenPosV2, p_topWallEnd);

    sf::RenderStates states;
    states.texture = &tex;

    states.shader = &m_perspectiveShader;

    m_perspectiveShader.setUniform("topLeft", quad[0].position);
    m_perspectiveShader.setUniform("bottomLeft", quad[1].position);
    m_perspectiveShader.setUniform("bottomRight", quad[2].position);
    m_perspectiveShader.setUniform("topRight", quad[3].position);
    m_perspectiveShader.setUniform("texSize", sf::Vector2f(static_cast<float>(texSize.x), static_cast<float>(texSize.y)));
    m_perspectiveShader.setUniform("wallLen", wallLen);
    m_perspectiveShader.setUniform("startRatio", wallRatioStart);
    m_perspectiveShader.setUniform("endRatio", wallRatioEnd);
    m_perspectiveShader.setUniform("leftDepth", 1.0f / singedPerpWorldDistanceStart);
    m_perspectiveShader.setUniform("rightDepth", 1.0f / singedPerpWorldDistanceEnd);
    m_perspectiveShader.setUniform("resolution", sf::Vector2f(m_engineSettings.resolution));

    m_renderTexture.draw(quad, states);
}

void TwoHalfD::Engine::renderSprite(const TwoHalfD::SpriteEntity &spriteEntity) {
    auto it = m_level.textures.find(spriteEntity.textureId);
    if (it == m_level.textures.end()) {
        std::cerr << "No texture found for sprite: " << spriteEntity.id << " with texture id: " << spriteEntity.textureId << std::endl;
        exit(1);
    }

    const sf::Texture &tex = it->second.texture;
    const sf::Vector2u texSize = tex.getSize();

    sf::Sprite sprite;
    sprite.setTexture(tex);
    sprite.setOrigin(texSize.x / 2.0f, texSize.y / 2.0f);

    float cameraDirRad = m_cameraObject.cameraPos.direction;
    sf::Vector2f direction{std::cos(cameraDirRad), std::sin(cameraDirRad)};
    sf::Vector2f n_plane{-direction.y, direction.x};
    sf::Vector2f plane{-direction.y * m_engineSettings.fovScale, direction.x * m_engineSettings.fovScale};

    float focalLength = (m_engineSettings.resolution.x / 2.0f) / m_engineSettings.fovScale;

    const sf::Vector2f spritePos = {spriteEntity.pos.posf.x, spriteEntity.pos.posf.y};
    const sf::Vector2f toSpriteVec = spritePos - sf::Vector2f(m_cameraObject.cameraPos.pos.x, m_cameraObject.cameraPos.pos.y);

    float perpWorldDistance = dotProduct(toSpriteVec, direction);

    if (perpWorldDistance <= 0) {
        return;
    }

    const float bottomOfSpriteScreen = focalLength * m_cameraObject.cameraHeight / perpWorldDistance + m_engineSettings.resolution.y / 2.0f;
    const float topSpriteScreen =
        focalLength * (m_cameraObject.cameraHeight - spriteEntity.height) / perpWorldDistance + m_engineSettings.resolution.y / 2.0f;

    const float spriteHeightScreen = bottomOfSpriteScreen - topSpriteScreen;

    const float spriteScreenX = (m_engineSettings.resolution.x / 2.0f) + focalLength * dotProduct(toSpriteVec, n_plane) / perpWorldDistance;

    sprite.setPosition(spriteScreenX, topSpriteScreen + spriteHeightScreen / 2.0f);
    sprite.setScale(spriteHeightScreen / texSize.y, spriteHeightScreen / texSize.y);
    float shade = std::min(1.0f, 256.0f * 1000.f / perpWorldDistance);
    sf::Uint8 shadeValue = static_cast<sf::Uint8>(255 * shade);
    sprite.setColor(sf::Color(shadeValue, shadeValue, shadeValue, 255));
    m_renderTexture.draw(sprite);
}

void TwoHalfD::Engine::renderWalls() {
    auto drawnCommands = m_bspManager.update(m_cameraObject.cameraPos);
    for (const auto &command : drawnCommands) {
        switch (command.type) {
        case TwoHalfD::DrawCommand::Type::Segment: {
            renderSegment(m_bspManager.getSegment(command.id));
            break;
        }
        case TwoHalfD::DrawCommand::Type::Sprite: {
            renderSprite(m_level.sprites[command.id]);
            break;
        }
        default:
            break;
        }
    }
}

void TwoHalfD::Engine::renderFloor() {
    static sf::Texture floorTileTexture;
    static sf::Image floorTileImage;
    static bool loaded = false;

    if (!loaded) {
        if (!floorTileTexture.loadFromFile(fs::path(ASSETS_DIR) / "textures" / "pattern_24.png")) {

            std::cerr << "Error loading floor texture." << std::endl;
            return;
        }
        floorTileImage = floorTileTexture.copyToImage();
        loaded = true;
    }

    float textureSize = floorTileTexture.getSize().x;
    if (floorTileTexture.getSize().x != floorTileTexture.getSize().y) {
        std::cout << "ERROR NOT SAME SIZE!!!!" << std::endl;
        exit(1);
    }

    const int numThreads = 4;
    const int startY = m_engineSettings.resolution.y / 2;
    const int endY = m_engineSettings.resolution.y;
    const int rowsPerThread = (endY - startY) / numThreads;

    std::vector<std::thread> threads;
    std::vector<sf::VertexArray> vertexArrays(numThreads, sf::VertexArray());

    sf::VertexArray floorVertices(sf::Points);
    floorVertices.resize(m_engineSettings.resolution.x * (m_engineSettings.resolution.y / 2));

    sf::Vector2f direction{std::cos(m_cameraObject.cameraPos.direction), std::sin(m_cameraObject.cameraPos.direction)};
    sf::Vector2f plane{-direction.y * m_engineSettings.fovScale, direction.x * m_engineSettings.fovScale};
    float focalLength = (m_engineSettings.resolution.x / 2.0f) / m_engineSettings.fovScale;

    for (int t = 0; t < numThreads; ++t) {

        int yStart = startY + t * rowsPerThread;
        int yEnd = (t == numThreads - 1) ? endY : yStart + rowsPerThread;

        threads.emplace_back([&, t, yStart, yEnd]() {
            vertexArrays[t].setPrimitiveType(sf::Points);
            vertexArrays[t].resize(m_engineSettings.resolution.x * (yEnd - yStart));

            for (int y = yStart; y < yEnd; ++y) {
                float pixelsFromCenterY = y - m_engineSettings.resolution.y / 2.0f;
                float perpWorldDistance = (m_cameraObject.cameraHeight * focalLength) / pixelsFromCenterY;

                if (perpWorldDistance > 3000.0f) {
                    continue;
                }

                float shade = std::min(1.0f, 256.0f / perpWorldDistance);

                float xCenter = 2.0f / static_cast<float>(m_engineSettings.resolution.x);
                for (int x = 0; x < m_engineSettings.resolution.x; ++x) {
                    float cameraX = xCenter * x - 1.0f;

                    sf::Vector2f rayDir = direction + plane * cameraX;

                    float realRayDist = perpWorldDistance / (rayDir.x * direction.x + rayDir.y * direction.y);
                    sf::Vector2f floorPos{m_cameraObject.cameraPos.pos.x + rayDir.x * realRayDist,
                                          m_cameraObject.cameraPos.pos.y + rayDir.y * realRayDist};

                    sf::Vector2f texPos = floorPos / textureSize;
                    sf::Vector2i cell{static_cast<int>(std::floor(texPos.x)), static_cast<int>(std::floor(texPos.y))};

                    sf::Vector2f texCoords = texPos - sf::Vector2f(cell);
                    sf::Vector2i texPixel{static_cast<int>(texCoords.x * textureSize), static_cast<int>(texCoords.y * textureSize)};

                    texPixel.x = texPixel.x % static_cast<int>(textureSize);
                    texPixel.y = texPixel.y % static_cast<int>(textureSize);

                    sf::Color color = floorTileImage.getPixel(static_cast<unsigned int>(texPixel.x), static_cast<unsigned int>(texPixel.y));

                    color.r *= shade;
                    color.g *= shade;
                    color.b *= shade;

                    sf::Vertex pixel(sf::Vector2f(x, y), color);
                    vertexArrays[t][(y - yStart) * m_engineSettings.resolution.x + x] = pixel;
                }
            }
        });
    }

    for (auto &thread : threads) {
        thread.join();
    }

    for (auto &va : vertexArrays) {
        m_renderTexture.draw(va);
    }
}

// <-------------- Physics -------------->
std::pair<const TwoHalfD::Wall *, float> TwoHalfD::Engine::findNearestWall(const XYVectorf &cord, const TwoHalfD::XYVectorf &rayDir,
                                                                           const std::vector<Wall> &walls) {
    const TwoHalfD::Wall *nearestWall = nullptr;
    float shortestDist = std::numeric_limits<float>::max();

    for (const auto &wall : walls) {
        float x1 = cord.x, y1 = cord.y;
        float x2 = x1 + 1000.0f * rayDir.x, y2 = y1 + 1000.0f * rayDir.y;

        float x3 = wall.start.x, y3 = wall.start.y;
        float x4 = wall.end.x, y4 = wall.end.y;

        float denom = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
        if (std::abs(denom) < 0.00001f) continue;

        float numeratorT = (x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4);
        float numeratorU = -((x1 - x2) * (y1 - y3) - (y1 - y2) * (x1 - x3));

        float t = numeratorT / denom;
        float u = numeratorU / denom;

        if (u < 0 || u > 1 || t < 0) continue;

        if (t < shortestDist) {
            shortestDist = t;
            nearestWall = &wall;
        }
    }
    return {nearestWall, nearestWall == nullptr ? std::numeric_limits<float>::max() : shortestDist * 1000};
}

std::pair<const TwoHalfD::Wall *, float> TwoHalfD::Engine::findNearestWall(const TwoHalfD::Position &ray, const std::vector<Wall> &walls) {
    XYVectorf dirVector{std::cos(ray.direction), std::sin(ray.direction)};
    return findNearestWall(ray.pos, dirVector, walls);
}

std::pair<const TwoHalfD::Wall *, float> TwoHalfD::Engine::findNearestWall(const TwoHalfD::Position &ray) {
    return findNearestWall(ray, getWallsInRegion());
}

// Collisions
const std::vector<const TwoHalfD::Wall *> TwoHalfD::Engine::wallCollisionSelf(const CameraObject &cameraObject) {

    auto &walls = getWallsInRegion();
    std::vector<const TwoHalfD::Wall *> wall_intercepts;

    for (const auto &wall : walls) {
        TwoHalfD::XYVectorf wallVector{wall.end.x - wall.start.x, wall.end.y - wall.start.y};
        TwoHalfD::XYVectorf perpWallVector{-wallVector.y, wallVector.x};
        float perpWallVectorLen = sqrt(perpWallVector.x * perpWallVector.x + perpWallVector.y * perpWallVector.y);
        TwoHalfD::XYVectorf perpWallVectorN{perpWallVector.x / perpWallVectorLen, perpWallVector.y / perpWallVectorLen};

        float x1 = cameraObject.cameraPos.posf.x, y1 = cameraObject.cameraPos.posf.y;
        float x2 = x1 + perpWallVectorN.x * cameraObject.cameraRadius, y2 = y1 + perpWallVectorN.y * cameraObject.cameraRadius;

        float x3 = wall.start.x, y3 = wall.start.y;
        float x4 = wall.end.x, y4 = wall.end.y;

        float denom = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
        if (std::abs(denom) < 0.00001f) continue;

        float numeratorT = (x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4);
        float numeratorU = -((x1 - x2) * (y1 - y3) - (y1 - y2) * (x1 - x3));

        float t = numeratorT / denom;
        float u = numeratorU / denom;

        if (u < 0 || u > 1 || t > 1 || t < -1) continue;

        wall_intercepts.push_back(&wall);
    }

    return wall_intercepts;
}

const std::vector<const TwoHalfD::Wall *> TwoHalfD::Engine::wallCollisionSelf() {

    auto &walls = getWallsInRegion();
    std::vector<const TwoHalfD::Wall *> wall_intercepts;

    const float r = m_cameraObject.cameraRadius;
    const float cx = m_cameraObject.cameraPos.posf.x, cy = m_cameraObject.cameraPos.posf.y;

    for (const auto &wall : walls) {
        // std::cout << "Wall is: id, (xs, ys), (xe, ye): " << wall.id << ", (" << wall.start.x << ", " << wall.start.y << "), (" << wall.end.x << ","
        //           << wall.end.y << ")\n";
        // std::cout << "(a, b, r): (" << a << ", " << b << ", " << c << ")\n";

        auto interceptPoints = findCircleLineSegmentIntercept(cx, cy, r, {wall.start.x, wall.start.y}, {wall.end.x, wall.end.y});

        if (interceptPoints.size() > 0) {
            wall_intercepts.push_back(&wall);
        }
    }

    return wall_intercepts;
}