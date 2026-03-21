#include "TwoHalfD/renderer.h"
#include "TwoHalfD/utils/math_util.h"

#include <SFML/Graphics/BlendMode.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/Graphics/PrimitiveType.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Vertex.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/System/Vector2.hpp>
#include <chrono>
#include <cmath>
#include <iostream>
#include <thread>

TwoHalfD::Renderer::Renderer(sf::RenderWindow &window, const EngineSettings &settings, EngineClocks &clocks)
    : m_window(window), m_settings(settings), m_clocks(clocks) {

    m_renderTexture.create(settings.resolution.x, settings.resolution.y);

    if (!sf::Shader::isAvailable()) {
        std::cerr << "Shaders not available!" << std::endl;
    }

    std::string shadersPath = static_cast<std::string>(ROOT_DIR) + "/engine/include/TwoHalfD/" + "shaders/perspectiveshader.frag";
    if (!m_perspectiveShader.loadFromFile(shadersPath, sf::Shader::Fragment)) {
        std::cerr << "Failed to load shader!" << std::endl;
        std::exit(1);
    }

    std::string floorShaderPath = static_cast<std::string>(ROOT_DIR) + "/engine/include/TwoHalfD/" + "shaders/floorShader.frag";
    if (!m_floorShader.loadFromFile(floorShaderPath, sf::Shader::Fragment)) {
        std::cerr << "Failed to load floor shader!" << std::endl;
        std::exit(1);
    }
}

void TwoHalfD::Renderer::render(const CameraObject &camera, Level &level, BSPManager &bsp) {
    if (!m_clocks.graphicsTimeDeltaPassed()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        return;
    }
    m_renderTexture.clear(sf::Color::Transparent);
    renderBSP(camera, level, bsp);
    renderOverlays(camera);

    m_renderTexture.display();
    sf::Sprite sprite(m_renderTexture.getTexture());
    sprite.setScale(static_cast<float>(m_settings.windowDim.x) / m_settings.resolution.x,
                    static_cast<float>(m_settings.windowDim.y) / m_settings.resolution.y);
    m_window.clear(sf::Color::Black);
    m_window.draw(sprite);
    m_window.display();
}

void TwoHalfD::Renderer::renderBSP(const CameraObject &camera, Level &level, BSPManager &bsp) {
    auto bspTraversal = bsp.update(const_cast<TwoHalfD::Position &>(camera.cameraPos));
    auto &drawnCommands = bspTraversal.first;

    renderFloor(camera, level);
    for (const auto &command : drawnCommands) {
        switch (command.type) {
        case TwoHalfD::DrawCommand::Type::Segment: {
            renderSegment(bsp.getSegment(command.id), camera, level);
            break;
        }
        case TwoHalfD::DrawCommand::Type::Sprite: {
            renderSprite(level.sprites[command.id], camera, level);
            break;
        }
        case TwoHalfD::DrawCommand::Type::FloorSection: {
            renderFloorSection(command.floorSectionPtr, camera, level);
            break;
        }
        default:
            break;
        }
    }
}

void TwoHalfD::Renderer::renderSegment(TwoHalfD::Segment segment, const CameraObject &camera, const Level &level) {
    const float NEAR_CLIP = 50.0f;

    auto wallB = segment.isWall() ? *segment.wall : TwoHalfD::Wall(segment.v1, segment.v2, 1, 1, segment.floorSection->height, 0);
    TwoHalfD::Wall *wall = &wallB;

    float p_focalLength = (m_settings.resolution.x / 2.0f) / m_settings.fovScale;
    TwoHalfD::XYVectorf n_direction{std::cos(camera.cameraPos.direction), std::sin(camera.cameraPos.direction)};
    TwoHalfD::XYVectorf n_plane{-n_direction.y, n_direction.x};

    TwoHalfD::XYVectorf vecCamV1 = segment.v1 - camera.cameraPos.pos;
    TwoHalfD::XYVectorf vecCamV2 = segment.v2 - camera.cameraPos.pos;

    float wallRatioStart = segment.wallRatioStart;
    float wallRatioEnd = segment.wallRatioEnd;

    float singedPerpWorldDistanceStart = dotProduct(vecCamV1, n_direction);
    float singedPerpWorldDistanceEnd = dotProduct(vecCamV2, n_direction);

    if (singedPerpWorldDistanceEnd < NEAR_CLIP && singedPerpWorldDistanceStart < NEAR_CLIP) {
        return;
    }

    float signedLateralDistV1 = dotProduct(vecCamV1, n_plane);
    float signedLateralDistV2 = dotProduct(vecCamV2, n_plane);

    bool v1RightV2Left = signedLateralDistV1 >= 0 && signedLateralDistV2 < 0;
    bool v2MoreLeft = std::signbit(signedLateralDistV1) == std::signbit(signedLateralDistV2) && signedLateralDistV1 > signedLateralDistV2;

    if (v1RightV2Left || v2MoreLeft) {
        std::swap(singedPerpWorldDistanceStart, singedPerpWorldDistanceEnd);
        std::swap(vecCamV1, vecCamV2);
        std::swap(signedLateralDistV1, signedLateralDistV2);
        std::swap(wallRatioStart, wallRatioEnd);
    }

    const float &halfYRes = m_settings.resolution.y / 2.f;
    const float &halfXRes = m_settings.resolution.x / 2.f;

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

    if ((p_xScreenPosV1 > m_settings.resolution.x && p_xScreenPosV2 > m_settings.resolution.x) ||
        (p_xScreenPosV2 < 0 && p_xScreenPosV1 < 0)) {
        return;
    }

    float p_topWallStart =
        p_focalLength * (camera.cameraHeight + camera.cameraHeightStart - wall->wallHeightStart - wall->height) / singedPerpWorldDistanceStart +
        halfYRes;
    float p_bottomWallStart =
        p_focalLength * (camera.cameraHeight + camera.cameraHeightStart - wall->wallHeightStart) / singedPerpWorldDistanceStart + halfYRes;

    float p_topWallEnd =
        p_focalLength * (camera.cameraHeight + camera.cameraHeightStart - wall->wallHeightStart - wall->height) / singedPerpWorldDistanceEnd +
        halfYRes;
    float p_bottomWallEnd =
        p_focalLength * (camera.cameraHeight + camera.cameraHeightStart - wall->wallHeightStart) / singedPerpWorldDistanceEnd + halfYRes;

    auto it = level.textures.find(wall->textureId);
    if (it == level.textures.end()) {
        std::cerr << "No texture found for wall: " << wall->id << " with texture id: " << wall->textureId << std::endl;
        exit(1);
    }

    const sf::Texture &tex = it->second.texture;
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
    m_perspectiveShader.setUniform("resolution", sf::Vector2f(m_settings.resolution));
    m_perspectiveShader.setUniform("shaderScale", m_settings.shaderScale);
    m_perspectiveShader.setUniform("wallHeightFloorHeighDiff", level.defaultFloorHeight - wall->wallHeightStart);
    m_perspectiveShader.setUniform("wallHeight", wall->height);

    m_renderTexture.draw(quad, states);
}

void TwoHalfD::Renderer::renderSprite(const TwoHalfD::SpriteEntity &spriteEntity, const CameraObject &camera, const Level &level) {
    auto it = level.textures.find(spriteEntity.textureId);
    if (it == level.textures.end()) {
        std::cerr << "No texture found for sprite: " << spriteEntity.id << " with texture id: " << spriteEntity.textureId << std::endl;
        exit(1);
    }

    const sf::Texture &tex = it->second.texture;
    const sf::Vector2u texSize = tex.getSize();

    sf::Sprite sprite;
    sprite.setTexture(tex);
    sprite.setOrigin(texSize.x / 2.0f, texSize.y / 2.0f);

    float cameraDirRad = camera.cameraPos.direction;
    sf::Vector2f direction{std::cos(cameraDirRad), std::sin(cameraDirRad)};
    sf::Vector2f n_plane{-direction.y, direction.x};

    float focalLength = (m_settings.resolution.x / 2.0f) / m_settings.fovScale;

    const sf::Vector2f spritePos = {spriteEntity.pos.pos.x, spriteEntity.pos.pos.y};
    const sf::Vector2f toSpriteVec = spritePos - camera.cameraPos.posf;

    float perpWorldDistance = dotProduct(toSpriteVec, direction);

    if (perpWorldDistance <= 0) {
        return;
    }

    const float bottomOfSpriteScreen =
        focalLength * (camera.cameraHeight + camera.cameraHeightStart - spriteEntity.heightStart) / perpWorldDistance +
        m_settings.resolution.y / 2.0f;
    const float topSpriteScreen =
        focalLength * (camera.cameraHeight + camera.cameraHeightStart - spriteEntity.height - spriteEntity.heightStart) / perpWorldDistance +
        m_settings.resolution.y / 2.0f;

    const float spriteHeightScreen = bottomOfSpriteScreen - topSpriteScreen;

    const float spriteScreenX = (m_settings.resolution.x / 2.0f) + focalLength * dotProduct(toSpriteVec, n_plane) / perpWorldDistance;

    sprite.setPosition(spriteScreenX, topSpriteScreen + spriteHeightScreen / 2.0f);
    sprite.setScale(spriteHeightScreen / texSize.y, spriteHeightScreen / texSize.y);
    float shade = std::min(1.0f, m_settings.shaderScale / perpWorldDistance);
    sf::Uint8 shadeValue = static_cast<sf::Uint8>(255 * shade);
    sprite.setColor(sf::Color(shadeValue, shadeValue, shadeValue, 255));
    m_renderTexture.draw(sprite);
}

void TwoHalfD::Renderer::renderFloorSection(const TwoHalfD::FloorSection *floorSection, const CameraObject &camera, const Level &level) {
    float focalLength = (m_settings.resolution.x / 2.0f) / m_settings.fovScale;
    XYVectorf n_direction{std::cos(camera.cameraPos.direction), std::sin(camera.cameraPos.direction)};
    XYVectorf n_plane{-n_direction.y, n_direction.x};
    const float NEAR_CLIP = 100.0f;

    auto texIt = level.textures.find(floorSection->textureId);
    if (texIt == level.textures.end()) {
        std::cerr << "No texture found for floor section with texture id: " << floorSection->textureId << std::endl;
        return;
    }
    const sf::Texture &floorTileTexture = texIt->second.texture;

    std::vector<XYVectorf> vertices{};
    size_t n = floorSection->vertices.size();
    vertices.reserve(n);

    for (size_t i{}; i < n; ++i) {
        const XYVectorf &curr = floorSection->vertices[i];
        const XYVectorf &next = floorSection->vertices[(i + 1) % n];

        float dotCurr = dotProduct(curr - camera.cameraPos.posf, n_direction);
        float dotNext = dotProduct(next - camera.cameraPos.posf, n_direction);

        bool currInFront = dotCurr > NEAR_CLIP;
        bool nextInFront = dotNext > NEAR_CLIP;

        if (currInFront) vertices.push_back(curr);

        if (currInFront != nextInFront) {
            float t = (NEAR_CLIP - dotCurr) / (dotNext - dotCurr);
            XYVectorf intersectionPoint = curr + t * (next - curr);
            vertices.push_back(intersectionPoint);
        }
    }

    sf::VertexArray floorShape(sf::PrimitiveType::TriangleFan, vertices.size());
    for (size_t i = 0; i < vertices.size(); ++i) {
        XYVectorf cameraVertexVec = vertices[i] - camera.cameraPos.posf;
        float perpWorldDistance = dotProduct(cameraVertexVec, n_direction);
        float lateralDist = dotProduct(cameraVertexVec, n_plane);
        float p_xScreenPos = (m_settings.resolution.x / 2.0f) + focalLength * lateralDist / perpWorldDistance;
        float p_yScreenPos = (m_settings.resolution.y / 2.0f) + focalLength * (camera.cameraHeight - floorSection->height) / perpWorldDistance;
        floorShape[i].position = sf::Vector2f(p_xScreenPos, p_yScreenPos);
    }

    sf::RenderStates states;
    states.texture = &floorTileTexture;
    states.shader = &m_floorShader;

    m_floorShader.setUniform("textureStartCord", sf::Vector2f(floorSection->floorTextureStart.x, floorSection->floorTextureStart.y));
    m_floorShader.setUniform("texture", floorTileTexture);
    m_floorShader.setUniform("textureSize", sf::Vector2f(floorTileTexture.getSize()));
    m_floorShader.setUniform("cameraPos", camera.cameraPos.posf);
    m_floorShader.setUniform("n_plane", sf::Vector2f(n_plane.x, n_plane.y));
    m_floorShader.setUniform("relativeCameraHeight", camera.cameraHeight + camera.cameraHeightStart - floorSection->height);
    m_floorShader.setUniform("direction", sf::Vector2f(n_direction.x, n_direction.y));
    m_floorShader.setUniform("focalLength", focalLength);
    m_floorShader.setUniform("resolution", sf::Vector2f(m_settings.resolution));
    m_floorShader.setUniform("distanceCutoff", 3000.0f);
    m_floorShader.setUniform("shaderScale", m_settings.shaderScale);

    m_renderTexture.draw(floorShape, states);
}

void TwoHalfD::Renderer::renderFloor(const CameraObject &camera, const Level &level) {
    float focalLength = (m_settings.resolution.x / 2.0f) / m_settings.fovScale;
    XYVectorf n_direction{std::cos(camera.cameraPos.direction), std::sin(camera.cameraPos.direction)};
    XYVectorf n_plane{-n_direction.y, n_direction.x};

    if (level.defaultFloorTextureId != -1) {
        auto it = level.textures.find(level.defaultFloorTextureId);
        if (it == level.textures.end()) {
            std::cerr << "No texture found for default floor with texture id: " << level.defaultFloorTextureId << std::endl;
            exit(1);
        }
        const sf::Texture &floorTileTexture = it->second.texture;

        sf::VertexArray quad(sf::Quads, 4);
        quad[0].position = sf::Vector2f(0, m_settings.resolution.y / 2.0f);
        quad[1].position = sf::Vector2f(0, m_settings.resolution.y);
        quad[2].position = sf::Vector2f(m_settings.resolution.x, m_settings.resolution.y);
        quad[3].position = sf::Vector2f(m_settings.resolution.x, m_settings.resolution.y / 2.0f);

        sf::RenderStates states;
        states.texture = &floorTileTexture;
        states.shader = &m_floorShader;

        m_floorShader.setUniform("textureStartCord", sf::Vector2f(level.defaultFloorStart.x, level.defaultFloorStart.y));
        m_floorShader.setUniform("texture", floorTileTexture);
        m_floorShader.setUniform("textureSize", sf::Vector2f(floorTileTexture.getSize()));
        m_floorShader.setUniform("cameraPos", sf::Vector2f(camera.cameraPos.pos.x, camera.cameraPos.pos.y));
        m_floorShader.setUniform("relativeCameraHeight", camera.cameraHeight + camera.cameraHeightStart - level.defaultFloorHeight);
        m_floorShader.setUniform("n_plane", sf::Vector2f(n_plane.x, n_plane.y));
        m_floorShader.setUniform("direction", sf::Vector2f(n_direction.x, n_direction.y));
        m_floorShader.setUniform("focalLength", focalLength);
        m_floorShader.setUniform("resolution", sf::Vector2f(m_settings.resolution));
        m_floorShader.setUniform("distanceCutoff", 3000.0f);
        m_floorShader.setUniform("shaderScale", m_settings.shaderScale);

        m_renderTexture.draw(quad, states);
    }
}

void TwoHalfD::Renderer::renderOverlays(const CameraObject &camera) {
    static bool loaded = false;
    static sf::Font font;
    if (!loaded) {
        font.loadFromFile(fs::path(ASSETS_DIR) / "fonts" / "RasterForgeRegular-JpBgm.ttf");
        loaded = true;
    }

    sf::Text text;
    text.setFont(font);
    std::string fpsString = "Fps: " + std::to_string(static_cast<int>(std::round(m_clocks.getAverageGraphicsFps())));
    text.setString(fpsString);
    text.setCharacterSize(24);
    text.setFillColor(sf::Color::Yellow);
    text.setPosition(m_settings.resolution.x - 120, m_settings.resolution.y - 50);
    m_renderTexture.draw(text);

    sf::Text text1;
    text1.setFont(font);
    std::string position = "(" + std::to_string(camera.cameraPos.pos.x) + ", " + std::to_string(camera.cameraPos.pos.y) + ")";
    text1.setString(position);
    text1.setCharacterSize(24);
    text1.setFillColor(sf::Color::Yellow);
    text1.setPosition(50, m_settings.resolution.y - 50);
    m_renderTexture.draw(text1);
}
