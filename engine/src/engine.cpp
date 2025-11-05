#include "TwoHalfD/engine_types.h"
#include "utils/mathUtil.h"
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Vertex.hpp>
#include <TwoHalfD/engine.h>
#include <cmath>
#include <cstdio>

void TwoHalfD::Engine::loadLevel(const Level &level)
{
    this->m_engineState = EngineState::fpsState;
    this->m_level = level;
    m_window.setMouseCursorVisible(false);

    m_cameraHeight = level.cameraHeightStart;

    // Add proper loading later
    // for (auto texture : m_level.textures) {

    //}
    sf::Texture tex;
    tex.loadFromFile("../assets/textures/pattern_18.png");
    tex.setRepeated(true);
    m_textures[1] = tex;

    Wall wall{1, {300, 256}, {1000, 256}, 256, 1};
    m_level.walls = {wall};
    Wall wall2{1, {0, 512}, {300, 256}, 256, 1};
    m_level.walls.push_back(wall2);
}

// Game Inputs
std::span<const TwoHalfD::Event> TwoHalfD::Engine::getFrameInputs()
{
    sf::Event event;
    while (m_window.pollEvent(event))
    {
        switch (event.type)
        {
        case sf::Event::Closed:
        {
            m_window.close();
            m_engineState = EngineState::ended;
            break;
        }
        case sf::Event::KeyPressed:
        {
            sf::Vector2i mouseWinPos = sf::Mouse::getPosition(m_window);
            m_inputArray[m_currentInput] = TwoHalfD::Event::KeyPressed(event.key.code, mouseWinPos.x, mouseWinPos.y);
            ++m_currentInput;
            break;
        }
        case sf::Event::KeyReleased:
        {
            sf::Vector2i mouseWinPos = sf::Mouse::getPosition(m_window);
            m_inputArray[m_currentInput] = TwoHalfD::Event::KeyReleased(event.key.code, mouseWinPos.x, mouseWinPos.y);
            ++m_currentInput;
            break;
        }
        case sf::Event::MouseMoved:
        {
            XYVector mouseDelta = m_engineContext.currentMousePosition - XYVector{event.mouseMove.x, event.mouseMove.y};
            m_engineContext.prevMousePosition = m_engineContext.currentMousePosition;
            m_engineContext.currentMousePosition = {event.mouseMove.x, event.mouseMove.y};
            m_inputArray[m_currentInput] = TwoHalfD::Event::MouseMoved(event.mouseMove.x, event.mouseMove.y, mouseDelta);
            ++m_currentInput;
        }
        default:
            break;
        }
    }
    backgroundFrameUpdates();
    return std::span<const TwoHalfD::Event>(m_inputArray.data(), m_currentInput);
}

void TwoHalfD::Engine::clearFrameInputs()
{
    m_currentInput = 0;
}

void TwoHalfD::Engine::backgroundFrameUpdates()
{
    // Calculate current context
    if (m_engineState == TwoHalfD::EngineState::fpsState)
    {
        XYVector middleScreen = {m_engineSettings.windowDim.x / 2, m_engineSettings.windowDim.y / 2};
        sf::Vector2i mouseWinPos = sf::Mouse::getPosition(m_window);
        XYVector mousePosition = {mouseWinPos.x, mouseWinPos.y};
        m_engineContext.MouseDelta = mousePosition - middleScreen;
        m_engineContext.currentMousePosition = middleScreen;
        m_engineContext.prevMousePosition = mousePosition;
    }

    // Reset stats for next iteration
    if (m_engineState == TwoHalfD::EngineState::fpsState)
    {
        if (m_window.hasFocus())
        {
            sf::Mouse::setPosition({m_engineContext.currentMousePosition.x, m_engineContext.currentMousePosition.y}, m_window);
        }
    }
}

TwoHalfD::XYVector TwoHalfD::Engine::getMouseDeltaFrame()
{
    return m_engineContext.MouseDelta;
}

// Getters and setters
TwoHalfD::WindowDim TwoHalfD::Engine::getWindowDimension()
{
    return {m_engineSettings.windowDim.x, m_engineSettings.windowDim.y};
}

TwoHalfD::Position TwoHalfD::Engine::getCameraPosition()
{
    return m_cameraPos;
}

void TwoHalfD::Engine::setCameraPosition(const TwoHalfD::Position &newPos)
{
    m_cameraPos = newPos;
}

void TwoHalfD::Engine::updateCameraPosition(const TwoHalfD::Position &posUpdate)
{
    m_cameraPos += posUpdate;
}

void TwoHalfD::Engine::setState(TwoHalfD::EngineState newState)
{
    m_engineState = newState;
}

TwoHalfD::EngineState TwoHalfD::Engine::getState()
{
    return this->m_engineState;
}

void TwoHalfD::Engine::renderAbove()
{
    m_window_above.clear(sf::Color::Black);

    for (int i = 0; i < 1920; i += 256)
    {
        sf::VertexArray lines(sf::LinesStrip, 2);
        lines[0].position = sf::Vector2f(i, 0);
        lines[1].position = sf::Vector2f(i, 1080);
        lines[0].color = sf::Color::Yellow;
        lines[1].color = sf::Color::Yellow;
        m_window_above.draw(lines);
    }
    for (int i = 0; i < 1080; i += 256)
    {
        sf::VertexArray lines(sf::LinesStrip, 2);
        lines[0].position = sf::Vector2f(0, i);
        lines[1].position = sf::Vector2f(1920, i);
        lines[0].color = sf::Color::Yellow;
        lines[1].color = sf::Color::Yellow;
        m_window_above.draw(lines);
    }

    for (auto wall : m_level.walls)
    {
        sf::VertexArray lines(sf::LinesStrip, 2);
        lines[0].position = sf::Vector2f(wall.start.x, wall.start.y);
        lines[1].position = sf::Vector2f(wall.end.x, wall.end.y);
        lines[0].color = sf::Color::Red;
        lines[1].color = sf::Color::Red;
        m_window_above.draw(lines);
    }
    TwoHalfD::Position camP = getCameraPosition();
    sf::CircleShape circle(10, 10);
    circle.setFillColor(sf::Color::White);
    circle.setPosition({100, 100});
    circle.setPosition(camP.pos.x - 10, camP.pos.y - 10);
    m_window_above.draw(circle);

    sf::VertexArray lines(sf::LinesStrip, 2);
    lines[0].position = sf::Vector2f(camP.pos.x, camP.pos.y);
    lines[1].position = sf::Vector2f(camP.pos.x + 20 * std::cos(camP.direction), camP.pos.y + 20 * std::sin(camP.direction));
    lines[0].color = sf::Color::Red;
    lines[1].color = sf::Color::Red;
    m_window_above.draw(lines);

    m_window_above.display();
}

// Render Logic
void TwoHalfD::Engine::render()
{
    renderAbove();
    m_window.clear(sf::Color::Black);
    m_renderTexture.clear(sf::Color::Black);
    this->renderFloor();
    this->renderWalls();

    m_renderTexture.display();
    sf::Sprite sprite(m_renderTexture.getTexture());
    sprite.setScale(static_cast<float>(m_engineSettings.windowDim.x) / m_engineSettings.resolution.x,
                    static_cast<float>(m_engineSettings.windowDim.y) / m_engineSettings.resolution.y);
    m_window.draw(sprite);
    m_window.display();
}

void TwoHalfD::Engine::renderWalls()
{
    float cameraDirRad = m_cameraPos.direction;
    sf::Vector2f direction{std::cos(cameraDirRad), std::sin(cameraDirRad)};
    sf::Vector2f plane{-direction.y * m_engineSettings.fovScale, direction.x * m_engineSettings.fovScale}; // Match floor renderer!

    float focalLength = (m_engineSettings.resolution.x / 2.0f) / m_engineSettings.fovScale;

    for (int x = 0; x < m_engineSettings.numRays; ++x)
    {
        float cameraX =
            2.0f * x * (1.0f * m_engineSettings.resolution.x / m_engineSettings.numRays) / static_cast<float>(m_engineSettings.resolution.x) - 1.0f;
        sf::Vector2f rayDir = direction + plane * cameraX;

        float rayLength = std::sqrt(rayDir.x * rayDir.x + rayDir.y * rayDir.y);
        rayDir.x /= rayLength;
        rayDir.y /= rayLength;

        float rayDirX = rayDir.x;
        float rayDirY = rayDir.y;

        float shortestDist = std::numeric_limits<float>::max();
        TwoHalfD::Wall *nearestWall = nullptr;

        for (auto &wall : m_level.walls)
        {
            float x1 = m_cameraPos.pos.x, y1 = m_cameraPos.pos.y;
            float x2 = x1 + 1000.0f * rayDirX, y2 = y1 + 1000.0f * rayDirY;

            float x3 = wall.start.x, y3 = wall.start.y;
            float x4 = wall.end.x, y4 = wall.end.y;

            float denom = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
            if (std::abs(denom) < 0.0001f)
                continue;

            float numeratorT = (x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4);
            float numeratorU = -((x1 - x2) * (y1 - y3) - (y1 - y2) * (x1 - x3));

            float t = numeratorT / denom;
            float u = numeratorU / denom;

            if (u < 0 || u > 1 || t < 0)
                continue;

            if (t < shortestDist)
            {
                shortestDist = t;
                nearestWall = &wall;
            }
        }

        if (nearestWall == nullptr)
            continue;

        auto it = m_textures.find(nearestWall->textureId);
        if (it == m_textures.end())
        {
            std::cerr << "No texture found for wall: " << nearestWall->id << " with texture id: " << nearestWall->textureId << std::endl;
            exit(1);
        }
        sf::Texture &tex = it->second;
        sf::Vector2u texSize = tex.getSize();

        float actualDistance = shortestDist * 1000.0f;

        float perpWorldDistance = actualDistance * (rayDirX * direction.x + rayDirY * direction.y);

        // move the following around by making (pixelsFromCenterY the subject)
        // float perpWorldDistance = (m_cameraHeight * focalLength) / pixelsFromCenterY;
        float bottomWall = focalLength * m_cameraHeight / perpWorldDistance + m_engineSettings.resolution.y / 2.0f;
        float topWall = focalLength * (m_cameraHeight - nearestWall->height) / perpWorldDistance + m_engineSettings.resolution.y / 2.0f;
        float wallHeight = bottomWall - topWall;

        float intersectX = m_cameraPos.pos.x + actualDistance * rayDirX;
        float intersectY = m_cameraPos.pos.y + actualDistance * rayDirY;

        float wallDirX = nearestWall->end.x - nearestWall->start.x;
        float wallDirY = nearestWall->end.y - nearestWall->start.y;
        float wallLen = std::sqrtf(wallDirX * wallDirX + wallDirY * wallDirY);

        wallDirX /= wallLen;
        wallDirY /= wallLen;

        float toIntersectX = intersectX - nearestWall->start.x;
        float toIntersectY = intersectY - nearestWall->start.y;

        float lenToIntercept = toIntersectX * wallDirX + toIntersectY * wallDirY;

        float texX = std::fmod(lenToIntercept, static_cast<float>(texSize.x));
        if (texX < 0)
            texX += texSize.x;

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

void TwoHalfD::Engine::renderFloor()
{
    static sf::Texture floorTileTexture;
    static sf::Image floorTileImage;
    static bool loaded = false;

    const char *floor_path{"../assets/textures/pattern_24.png"};

    if (!loaded)
    {
        if (!floorTileTexture.loadFromFile(floor_path))
        {
            std::cout << "Error loading floor texture." << std::endl;
            return;
        }
        floorTileImage = floorTileTexture.copyToImage();
        loaded = true;
    }

    sf::Vector2f direction{std::cos(m_cameraPos.direction), std::sin(m_cameraPos.direction)};
    sf::Vector2f plane{-direction.y * m_engineSettings.fovScale, direction.x * m_engineSettings.fovScale};

    sf::VertexArray floorVertices(sf::Points);
    floorVertices.resize(m_engineSettings.resolution.x * (m_engineSettings.resolution.y / 2));

    float textureSize = floorTileTexture.getSize().x;
    if (floorTileTexture.getSize().x != floorTileTexture.getSize().y)
    {
        std::cout << "ERROR NOT SAME SIZE!!!!" << std::endl;
        exit(1);
    }

    float focalLength = (m_engineSettings.resolution.x / 2.0f) / m_engineSettings.fovScale;

    for (int y = m_engineSettings.resolution.y / 2; y < m_engineSettings.resolution.y; ++y)
    {
        float pixelsFromCenterY = y - m_engineSettings.resolution.y / 2.0f;

        float perpWorldDistance = (m_cameraHeight * focalLength) / pixelsFromCenterY;

        if (perpWorldDistance > 3000.0f)
        {
            continue;
        }

        for (int x = 0; x < m_engineSettings.resolution.x; ++x)
        {
            float cameraX = 2.0f * x / static_cast<float>(m_engineSettings.resolution.x) - 1.0f;
            sf::Vector2f rayDir = direction + plane * cameraX;

            float rayLength = std::sqrt(rayDir.x * rayDir.x + rayDir.y * rayDir.y);
            rayDir.x /= rayLength;
            rayDir.y /= rayLength;

            float realRayDist = perpWorldDistance / (rayDir.x * direction.x + rayDir.y * direction.y);
            sf::Vector2f floorPos{m_cameraPos.pos.x + rayDir.x * realRayDist, m_cameraPos.pos.y + rayDir.y * realRayDist};

            sf::Vector2f texPos = floorPos / textureSize;
            sf::Vector2i cell{static_cast<int>(std::floor(texPos.x)), static_cast<int>(std::floor(texPos.y))};

            sf::Vector2f texCoords = texPos - sf::Vector2f(cell);
            sf::Vector2i texPixel{static_cast<int>(texCoords.x * textureSize), static_cast<int>(texCoords.y * textureSize)};

            texPixel.x = texPixel.x % static_cast<int>(textureSize);
            texPixel.x = texPixel.x % static_cast<int>(textureSize);
            // texPixel.x = texPixel.x & (static_cast<int>(textureSize) - 1); // Same only if floor size is a power of 2
            // texPixel.y = texPixel.y & (static_cast<int>(textureSize) - 1);

            sf::Color color = floorTileImage.getPixel(static_cast<unsigned int>(texPixel.x), static_cast<unsigned int>(texPixel.y));

            float shade = std::min(1.0f, 256.0f / perpWorldDistance);
            color.r *= shade;
            color.g *= shade;
            color.b *= shade;

            sf::Vertex pixel(sf::Vector2f(x, y), color);
            floorVertices[(y - m_engineSettings.resolution.y / 2) * m_engineSettings.resolution.x + x] = pixel;
        }
    }

    m_renderTexture.draw(floorVertices);
}