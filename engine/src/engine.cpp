#include "TwoHalfD/engine_types.h"
#include "utils/math_util.h"

#include <SFML/Graphics/BlendMode.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/PrimitiveType.hpp>
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

// In backgroundFrameUpdates:
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
    const float moveVecLen = moveVec.length();
    TwoHalfD::XYVectorf n_moveVec{moveVec.normalized()};

    float moveMagnitude = 0;
    if (m_engineSettings.cameraCollision) {
        TwoHalfD::XYVectorf oldPos = prevPos.pos;
        auto wallReferences = wallCollisionSelf();
        if (wallReferences.size() > 0) {
            for (auto wall : wallReferences) {
                std::cout << "Wall intercept: " << wall->id << '\n';
                const TwoHalfD::XYVectorf wallVec = {(wall->end.x - wall->start.x), (wall->end.y - wall->start.y)};
                TwoHalfD::XYVectorf n_wallVec{wallVec.normalized()};

                const float perpPointDistToStart = TwoHalfD::dot(m_cameraObject.cameraPos.pos - wall->start, n_wallVec);
                TwoHalfD::XYVectorf perpP{wall->start + n_wallVec * perpPointDistToStart};

                TwoHalfD::XYVectorf perpVec{m_cameraObject.cameraPos.pos - perpP};
                TwoHalfD::XYVectorf n_perpVec{perpVec.normalized()};

                TwoHalfD::XYVectorf oldPerpVec{oldPos - perpP};
                float oldPerpVecLen = oldPerpVec.length();
                float perpVecLen = perpVec.length();

                float penetrationDepth;
                if (perpVecLen < oldPerpVecLen) {
                    penetrationDepth = m_cameraObject.cameraRadius - perpVecLen;
                } else {
                    penetrationDepth = m_cameraObject.cameraRadius + perpVecLen;
                }
                float moveRatio = std::abs(dot(n_perpVec, n_moveVec));
                if (std::abs(moveRatio) < 0.01f) continue;

                moveMagnitude = std::min(std::max(moveMagnitude, penetrationDepth / moveRatio), moveVecLen);
                std::cout << "perpVecLen: " << perpVecLen << ", penetrationDepth: " << penetrationDepth << ", moveRatio: " << moveRatio
                          << ", moveMagnitude: " << moveMagnitude << '\n';
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

// void TwoHalfD::Engine::renderAbove()
// {
//     m_window_above.clear(sf::Color::Black);

//     for (int i = 0; i < 1920; i += 256)
//     {
//         sf::VertexArray lines(sf::LinesStrip, 2);
//         lines[0].position = sf::Vector2f(i, 0);
//         lines[1].position = sf::Vector2f(i, 1080);
//         lines[0].color = sf::Color::Yellow;
//         lines[1].color = sf::Color::Yellow;
//         m_window_above.draw(lines);
//     }
//     for (int i = 0; i < 1080; i += 256)
//     {
//         sf::VertexArray lines(sf::LinesStrip, 2);
//         lines[0].position = sf::Vector2f(0, i);
//         lines[1].position = sf::Vector2f(1920, i);
//         lines[0].color = sf::Color::Yellow;
//         lines[1].color = sf::Color::Yellow;
//         m_window_above.draw(lines);
//     }

//     for (auto wall : m_level.walls)
//     {
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
    // renderAbove();
    m_window.clear(sf::Color::Black);
    m_renderTexture.clear(sf::Color::Transparent);
    renderFloor();
    renderWalls();
    renderObjects();
    renderOverlays();

    m_renderTexture.display();
    sf::Sprite sprite(m_renderTexture.getTexture());
    sprite.setScale(static_cast<float>(m_engineSettings.windowDim.x) / m_engineSettings.resolution.x,
                    static_cast<float>(m_engineSettings.windowDim.y) / m_engineSettings.resolution.y);
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

void TwoHalfD::Engine::renderObjects() {
    float cameraDirRad = m_cameraObject.cameraPos.direction;
    sf::Vector2f direction{std::cos(cameraDirRad), std::sin(cameraDirRad)};
    sf::Vector2f plane{-direction.y * m_engineSettings.fovScale, direction.x * m_engineSettings.fovScale};
    const float planeLen = std::sqrt(plane.x * plane.x + plane.y * plane.y);
    const sf::Vector2f normalizedPlane = {plane.x / planeLen, plane.y / planeLen};

    float focalLength = (m_engineSettings.resolution.x / 2.0f) / m_engineSettings.fovScale;

    for (int x = 0; x < m_engineSettings.numRays; ++x) {
        float cameraX =
            2.0f * x * (1.0f * m_engineSettings.resolution.x / m_engineSettings.numRays) / static_cast<float>(m_engineSettings.resolution.x) - 1.0f;
        sf::Vector2f rayDir = direction + plane * cameraX;
        float rayLength = std::sqrt(rayDir.x * rayDir.x + rayDir.y * rayDir.y);
        rayDir.x /= rayLength;
        rayDir.y /= rayLength;

        float rayDirX = rayDir.x;
        float rayDirY = rayDir.y;

        auto cmp = [](const auto &a, const auto &b) { return a.first < b.first; };

        std::priority_queue<std::pair<float, TwoHalfD::SpriteEntity>, std::vector<std::pair<float, TwoHalfD::SpriteEntity>>, decltype(cmp)>
            spriteOrderedDistance(cmp);
        for (const auto &object : getSpriteEntitiesInRegion()) {
            if (object.textureId == -1) continue;
            // https://en.wikipedia.org/wiki/Line–line_intersection
            const float x1 = m_cameraObject.cameraPos.pos.x, y1 = m_cameraObject.cameraPos.pos.y;
            const float x2 = x1 + 1000.0f * rayDirX, y2 = y1 + 1000.0f * rayDirY;

            const float x3 = object.pos.posf.x + object.radius * normalizedPlane.x, y3 = object.pos.posf.y + object.radius * normalizedPlane.y;
            const float x4 = object.pos.posf.x - object.radius * normalizedPlane.x, y4 = object.pos.posf.y - object.radius * normalizedPlane.y;

            float denom = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
            if (std::abs(denom) < 0.00001f) continue;

            float numeratorT = (x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4);
            float numeratorU = -((x1 - x2) * (y1 - y3) - (y1 - y2) * (x1 - x3));

            float t = numeratorT / denom;
            float u = numeratorU / denom;

            if (u < 0 || u > 1 || t < 0 || t * 1000 >= m_renderZBuffer.nearestWallRayDist[x]) continue;

            spriteOrderedDistance.push({t * 1000.0f, object});
        }

        while (!spriteOrderedDistance.empty()) {
            const auto distSpritePair = spriteOrderedDistance.top();
            spriteOrderedDistance.pop();
            const float distToSprite = distSpritePair.first;
            const TwoHalfD::SpriteEntity currSprite = distSpritePair.second;
            const auto &textureIt = m_level.textures.find(currSprite.textureId);
            if (textureIt == m_level.textures.end()) {
                exit(1);
            }
            const sf::Texture &tex = textureIt->second.texture;
            const sf::Vector2u texSize = tex.getSize();

            float perpWorldDistance = distToSprite * (rayDirX * direction.x + rayDirY * direction.y);

            const float bottomOfSpriteScreen = focalLength * m_cameraObject.cameraHeight / perpWorldDistance + m_engineSettings.resolution.y / 2.0f;
            const float topSpriteScreen =
                focalLength * (m_cameraObject.cameraHeight - currSprite.height) / perpWorldDistance + m_engineSettings.resolution.y / 2.0f;

            const float spriteHeightScreen = bottomOfSpriteScreen - topSpriteScreen;

            const sf::Vector2f spriteStartPos = {currSprite.pos.posf.x - currSprite.radius * normalizedPlane.x,
                                                 currSprite.pos.posf.y - currSprite.radius * normalizedPlane.y};
            const sf::Vector2f spriteEndPos = {currSprite.pos.posf.x + currSprite.radius * normalizedPlane.x,
                                               currSprite.pos.posf.y + currSprite.radius * normalizedPlane.y};

            const float intersectX = m_cameraObject.cameraPos.pos.x + distToSprite * rayDirX;
            const float intersectY = m_cameraObject.cameraPos.pos.y + distToSprite * rayDirY;

            float spriteDirX = spriteEndPos.x - spriteStartPos.x;
            float spriteDirY = spriteEndPos.y - spriteStartPos.y;
            const float wallLen = std::sqrtf(spriteDirX * spriteDirX + spriteDirY * spriteDirY);

            spriteDirX /= wallLen;
            spriteDirY /= wallLen;

            const float toIntersectX = intersectX - spriteStartPos.x;
            const float toIntersectY = intersectY - spriteStartPos.y;

            float lenToIntercept = toIntersectX * spriteDirX + toIntersectY * spriteDirY;
            float texX = (lenToIntercept / (2.0f * currSprite.radius)) * texSize.x;

            texX = std::max(0.0f, std::min(texX, static_cast<float>(texSize.x - 1)));

            int sliceWidth = m_engineSettings.resolution.x / m_engineSettings.numRays;

            sf::Sprite sprite;
            sprite.setTexture(tex);
            sf::IntRect subRect(static_cast<int>(texX), 0, sliceWidth, texSize.y);
            sprite.setTextureRect(subRect);
            sprite.setScale(1.0f, spriteHeightScreen / texSize.y);
            sprite.setPosition(x * sliceWidth, topSpriteScreen);

            float shade = std::min(1.0f, 256.0f / distToSprite);
            sf::Uint8 shadeValue = static_cast<sf::Uint8>(255 * shade);

            sprite.setColor(sf::Color(shadeValue, shadeValue, shadeValue, 255));

            m_renderTexture.draw(sprite);
        }
    }
}

void TwoHalfD::Engine::renderWalls() {
    float cameraDirRad = m_cameraObject.cameraPos.direction;
    sf::Vector2f direction{std::cos(cameraDirRad), std::sin(cameraDirRad)};
    sf::Vector2f plane{-direction.y * m_engineSettings.fovScale, direction.x * m_engineSettings.fovScale};

    float focalLength = (m_engineSettings.resolution.x / 2.0f) / m_engineSettings.fovScale;

    for (int x = 0; x < m_engineSettings.numRays; ++x) {
        float cameraX =
            2.0f * x * (1.0f * m_engineSettings.resolution.x / m_engineSettings.numRays) / static_cast<float>(m_engineSettings.resolution.x) - 1.0f;
        sf::Vector2f rayDir = direction + plane * cameraX;

        float rayLength = std::sqrt(rayDir.x * rayDir.x + rayDir.y * rayDir.y);
        rayDir.x /= rayLength;
        rayDir.y /= rayLength;

        float rayDirX = rayDir.x;
        float rayDirY = rayDir.y;

        float shortestDist = std::numeric_limits<float>::max();
        // Test opt removing pointer later
        TwoHalfD::Wall *nearestWall = nullptr;

        for (auto &wall : getWallsInRegion()) {
            // https://en.wikipedia.org/wiki/Line–line_intersection
            float x1 = m_cameraObject.cameraPos.pos.x, y1 = m_cameraObject.cameraPos.pos.y;
            float x2 = x1 + 1000.0f * rayDirX, y2 = y1 + 1000.0f * rayDirY;

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

        m_renderZBuffer.nearestWallRayDist[x] = std::numeric_limits<float>::max();

        if (nearestWall == nullptr) continue;

        float actualDistance = shortestDist * 1000.0f;
        m_renderZBuffer.nearestWallRayDist[x] = actualDistance;
        float perpWorldDistance = actualDistance * (rayDirX * direction.x + rayDirY * direction.y);

        auto it = m_level.textures.find(nearestWall->textureId);
        if (it == m_level.textures.end()) {
            std::cerr << "No texture found for wall: " << nearestWall->id << " with texture id: " << nearestWall->textureId << std::endl;
            exit(1);
        }
        sf::Texture &tex = it->second.texture;
        sf::Vector2u texSize = tex.getSize();

        // move the following around by making (pixelsFromCenterY the subject)
        // float perpWorldDistance = (m_cameraObject.cameraHeight * focalLength) / pixelsFromCenterY;
        float bottomWall = focalLength * m_cameraObject.cameraHeight / perpWorldDistance + m_engineSettings.resolution.y / 2.0f;
        float topWall = focalLength * (m_cameraObject.cameraHeight - nearestWall->height) / perpWorldDistance + m_engineSettings.resolution.y / 2.0f;
        float wallHeight = bottomWall - topWall;

        float intersectX = m_cameraObject.cameraPos.pos.x + actualDistance * rayDirX;
        float intersectY = m_cameraObject.cameraPos.pos.y + actualDistance * rayDirY;

        float wallDirX = nearestWall->end.x - nearestWall->start.x;
        float wallDirY = nearestWall->end.y - nearestWall->start.y;
        float wallLen = std::sqrtf(wallDirX * wallDirX + wallDirY * wallDirY);

        wallDirX /= wallLen;
        wallDirY /= wallLen;

        float toIntersectX = intersectX - nearestWall->start.x;
        float toIntersectY = intersectY - nearestWall->start.y;

        float lenToIntercept = toIntersectX * wallDirX + toIntersectY * wallDirY;

        float texX = std::fmod(lenToIntercept, static_cast<float>(texSize.x));
        if (texX < 0) texX += texSize.x;

        int sliceWidth = m_engineSettings.resolution.x / m_engineSettings.numRays;

        sf::Sprite sprite;
        sprite.setTexture(tex);
        sf::IntRect subRect(static_cast<int>(texX), 0, sliceWidth, texSize.y);
        sprite.setTextureRect(subRect);
        sprite.setScale(1.0f, wallHeight / texSize.y);
        sprite.setPosition(x * sliceWidth, topWall);

        float shade = std::min(1.0f, 256.0f / actualDistance);
        sprite.setColor(
            sf::Color(static_cast<sf::Uint8>(255 * shade), static_cast<sf::Uint8>(255 * shade), static_cast<sf::Uint8>(255 * shade), 255));
        m_renderTexture.draw(sprite);
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

// Physics
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
            std::cout << "WALL ID IN INTERCEPT: " << wall.id << "\n";
            wall_intercepts.push_back(&wall);
        }
    }

    return wall_intercepts;
}