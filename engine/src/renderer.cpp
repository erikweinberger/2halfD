#include "TwoHalfD/renderer.h"
#include "TwoHalfD/bsp/bsp_manager.h"
#include "TwoHalfD/engine_types.h"
#include "TwoHalfD/types/entity_types.h"
#include "TwoHalfD/types/input_types.h"
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
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <sys/_types/_u_int32_t.h>
#include <thread>

const float NEAR_CLIP = 1.0f;

TwoHalfD::Renderer::Renderer(sf::RenderWindow &window, const EngineSettings &settings, EngineClocks &clocks)
    : m_window(window), m_settings(settings), m_clocks(clocks) {

    if (!m_renderTexture.resize({static_cast<unsigned>(settings.resolution.x), static_cast<unsigned>(settings.resolution.y)})) {
        std::cerr << "Failed to create render texture!" << std::endl;
        std::exit(1);
    }

    if (!sf::Shader::isAvailable()) {
        std::cerr << "Shaders not available!" << std::endl;
    }

    std::string shadersPath = static_cast<std::string>(ROOT_DIR) + "/engine/include/TwoHalfD/" + "shaders/perspectiveshader.frag";
    if (!m_perspectiveShader.loadFromFile(shadersPath, sf::Shader::Type::Fragment)) {
        std::cerr << "Failed to load shader!" << std::endl;
        std::exit(1);
    }

    std::string floorShaderPath = static_cast<std::string>(ROOT_DIR) + "/engine/include/TwoHalfD/" + "shaders/floorShader.frag";
    if (!m_floorShader.loadFromFile(floorShaderPath, sf::Shader::Type::Fragment)) {
        std::cerr << "Failed to load floor shader!" << std::endl;
        std::exit(1);
    }
    std::string fontPath = static_cast<std::string>(ASSETS_DIR) + "/fonts/JetBrainsMono-2/fonts/ttf/JetBrainsMono-Regular.ttf";
    if (!m_debugSettings.debugFont.openFromFile(fontPath)) {
        std::cerr << "Failed to load font!" << std::endl;
        std::exit(1);
    }
    m_debugSettings.cachedText.emplace(m_debugSettings.debugFont);
    m_debugSettings.debugColor = sf::Color(0, 255, 0, 255);
    m_debugSettings.fontSize = std::min(std::max(m_window.getSize().y / 40, m_debugSettings.minFontSize), m_debugSettings.maxFontSize);
}

void TwoHalfD::Renderer::setData(const std::unordered_map<int, TextureSignature> *textures, const EntityManager *entityManager,
                                 float defaultFloorHeight, int defaultFloorTextureId, XYVectorf defaultFloorStart) {
    m_textures = textures;
    m_entityManager = entityManager;
    m_defaultFloorHeight = defaultFloorHeight;
    m_defaultFloorTextureId = defaultFloorTextureId;
    m_defaultFloorStart = defaultFloorStart;
}

void TwoHalfD::Renderer::render(const CameraObject &camera, BSPManager &bsp) {
    if (!m_clocks.graphicsTimeDeltaPassed()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        return;
    }
    m_renderTexture.clear(sf::Color::Transparent);
    renderBSP(camera, bsp);
    renderOverlays(camera);
    renderDebugOverlays(camera, std::nullopt, bsp);

    m_renderTexture.display();
    sf::Sprite sprite(m_renderTexture.getTexture());
    sprite.setScale(
        {static_cast<float>(m_settings.windowDim.x) / m_settings.resolution.x, static_cast<float>(m_settings.windowDim.y) / m_settings.resolution.y});
    m_window.clear(sf::Color::Black);
    m_window.draw(sprite);
    m_window.display();
}

void TwoHalfD::Renderer::renderBSP(const CameraObject &camera, BSPManager &bsp) {
    auto bspTraversal = bsp.update(const_cast<TwoHalfD::Position &>(camera.cameraPos));
    auto &drawnCommands = bspTraversal.first;

    renderFloor(camera);
    m_debugSettings.numSpritesRenderedFrame = 0u;
    m_debugSettings.numSegmentsRenderedFrame = 0u;
    m_debugSettings.numFloorSectionsRenderedFrame = 0u;
    m_debugSettings.numColourOverlaysRenderedFrame = 0u;
    m_debugSettings.numEffectsRenderedFrame = 0u;
    for (const auto &command : drawnCommands) {
        switch (command.type) {
        case TwoHalfD::DrawCommand::Type::Segment: {
            renderSegment(bsp.getSegment(command.id), camera);
            break;
        }
        case TwoHalfD::DrawCommand::Type::Sprite: {
            auto entity = m_entityManager->getEntity(command.id);
            if (entity) renderSprite(*entity, camera);
            break;
        }
        case TwoHalfD::DrawCommand::Type::FloorSection: {
            renderFloorSection(command.floorSectionPtr, camera);
            break;
        }
        case TwoHalfD::DrawCommand::Type::Effect: {
            const auto &effects = m_entityManager->getAllEffects();
            auto it = effects.find(command.id);
            if (it != effects.end()) renderEffect(it->second, camera);
            break;
        }
        case TwoHalfD::DrawCommand::Type::ColourOverlay: {
            renderColourOverlay(command.colourOverlayPtr, camera);
            break;
        }
        default:
            break;
        }
    }
}

void TwoHalfD::Renderer::renderSegment(const TwoHalfD::Segment &segment, const CameraObject &camera) {

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

    if ((p_xScreenPosV1 > m_settings.resolution.x && p_xScreenPosV2 > m_settings.resolution.x) || (p_xScreenPosV2 < 0 && p_xScreenPosV1 < 0)) {
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

    auto it = m_textures->find(wall->textureId);
    if (it == m_textures->end()) {
        std::cerr << "No texture found for wall: " << wall->id << " with texture id: " << wall->textureId << std::endl;
        exit(1);
    }

    const sf::Texture &tex = it->second.texture;

    sf::VertexArray quad(sf::PrimitiveType::TriangleFan, 4);

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
    m_perspectiveShader.setUniform("startRatio", wallRatioStart);
    m_perspectiveShader.setUniform("endRatio", wallRatioEnd);
    m_perspectiveShader.setUniform("leftDepth", 1.0f / singedPerpWorldDistanceStart);
    m_perspectiveShader.setUniform("rightDepth", 1.0f / singedPerpWorldDistanceEnd);
    m_perspectiveShader.setUniform("resolution", sf::Vector2f(m_settings.resolution));
    m_perspectiveShader.setUniform("shaderScale", m_settings.shaderScale);
    m_perspectiveShader.setUniform("wallHeightFloorHeighDiff", m_defaultFloorHeight - wall->wallHeightStart);
    m_perspectiveShader.setUniform("wallHeight", wall->height);
    m_perspectiveShader.setUniform("scaleX", wall->scaleX);
    m_perspectiveShader.setUniform("scaleY", wall->scaleY);

    m_renderTexture.draw(quad, states);
    ++m_debugSettings.numSegmentsRenderedFrame;
}

void TwoHalfD::Renderer::renderSprite(const TwoHalfD::SpriteEntity &spriteEntity, const CameraObject &camera) {
    int textureId = spriteEntity.textureId;

    if (spriteEntity.currentAnimation) {
        const auto &animState = *spriteEntity.currentAnimation;
        const auto *templates = m_entityManager->getAnimationTemplates();
        if (templates) {
            auto tmplIt = templates->find(animState.templateId);
            if (tmplIt != templates->end() && !tmplIt->second.frames.empty()) {
                textureId = tmplIt->second.frames[animState.frameIndex].textureId;
            }
        }
    }

    auto it = m_textures->find(textureId);
    if (it == m_textures->end()) {
        std::cerr << "No texture found for sprite: " << spriteEntity.id << " with texture id: " << textureId << std::endl;
        exit(1);
    }

    const sf::Texture &tex = it->second.texture;
    const sf::Vector2u texSize = tex.getSize();

    int tiledW = static_cast<int>(texSize.x / spriteEntity.scaleX);
    int tiledH = static_cast<int>(texSize.y / spriteEntity.scaleY);

    sf::Sprite sprite(tex);
    sprite.setTextureRect(sf::IntRect({0, 0}, {tiledW, tiledH}));
    sprite.setOrigin({tiledW / 2.0f, tiledH / 2.0f});

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

    const float bottomOfSpriteScreen = focalLength * (camera.cameraHeight + camera.cameraHeightStart - spriteEntity.heightStart) / perpWorldDistance +
                                       m_settings.resolution.y / 2.0f;
    const float topSpriteScreen =
        focalLength * (camera.cameraHeight + camera.cameraHeightStart - spriteEntity.height - spriteEntity.heightStart) / perpWorldDistance +
        m_settings.resolution.y / 2.0f;

    const float spriteHeightScreen = bottomOfSpriteScreen - topSpriteScreen;

    const float spriteScreenX = (m_settings.resolution.x / 2.0f) + focalLength * dotProduct(toSpriteVec, n_plane) / perpWorldDistance;

    sprite.setPosition({spriteScreenX, topSpriteScreen + spriteHeightScreen / 2.0f});
    sprite.setScale({spriteHeightScreen / tiledH, spriteHeightScreen / tiledH});
    float shade = std::min(1.0f, m_settings.shaderScale / perpWorldDistance);
    std::uint8_t shadeValue = static_cast<std::uint8_t>(255 * shade);
    sprite.setColor(sf::Color(shadeValue, shadeValue, shadeValue, 255));
    m_renderTexture.draw(sprite);
    ++m_debugSettings.numSpritesRenderedFrame;

    // Render overlays (already sorted by zOrder)
    const auto *templates = m_entityManager->getAnimationTemplates();
    if (!templates) return;

    const float spriteWidthScreen = spriteHeightScreen * (static_cast<float>(texSize.x) / texSize.y);
    const float spriteLeft = spriteScreenX - spriteWidthScreen / 2.0f;
    const float spriteTop = topSpriteScreen;

    for (size_t i = 0; i < spriteEntity.overlays.count; ++i) {
        const auto &overlay = spriteEntity.overlays.overlays[i];
        if (!overlay.active) continue;

        auto tmplIt = templates->find(overlay.animState.templateId);
        if (tmplIt == templates->end() || tmplIt->second.frames.empty()) continue;

        int overlayTexId = tmplIt->second.frames[overlay.animState.frameIndex].textureId;
        auto overlayTexIt = m_textures->find(overlayTexId);
        if (overlayTexIt == m_textures->end()) continue;

        const sf::Texture &overlayTex = overlayTexIt->second.texture;
        const sf::Vector2u overlayTexSize = overlayTex.getSize();

        int tiledW = static_cast<int>(overlayTexSize.x / overlay.textureScaleX);
        int tiledH = static_cast<int>(overlayTexSize.y / overlay.textureScaleY);

        sf::Sprite overlaySprite(overlayTex);
        overlaySprite.setTextureRect(sf::IntRect({0, 0}, {tiledW, tiledH}));
        overlaySprite.setOrigin({tiledW / 2.0f, tiledH / 2.0f});
        float overlayWidthScreen = focalLength * overlay.width / perpWorldDistance;
        float overlayHeightScreen = focalLength * overlay.height / perpWorldDistance;
        overlaySprite.setScale({overlayWidthScreen / tiledW, overlayHeightScreen / tiledH});

        float ox = spriteLeft + overlay.x * spriteWidthScreen;
        float oy = spriteTop + overlay.y * spriteHeightScreen;
        overlaySprite.setPosition({ox, oy});

        overlaySprite.setColor(sf::Color(shadeValue, shadeValue, shadeValue, 255));
        m_renderTexture.draw(overlaySprite);
        ++m_debugSettings.numEffectsRenderedFrame;
    }
}

void TwoHalfD::Renderer::renderEffect(const TwoHalfD::AnimationEffect &effect, const CameraObject &camera) {
    const auto *templates = m_entityManager->getAnimationTemplates();
    if (!templates) return;

    auto tmplIt = templates->find(effect.animState.templateId);
    if (tmplIt == templates->end() || tmplIt->second.frames.empty()) return;

    int texId = tmplIt->second.frames[effect.animState.frameIndex].textureId;
    auto texIt = m_textures->find(texId);
    if (texIt == m_textures->end()) return;

    const sf::Texture &tex = texIt->second.texture;
    const sf::Vector2u texSize = tex.getSize();

    float cameraDirRad = camera.cameraPos.direction;
    sf::Vector2f direction{std::cos(cameraDirRad), std::sin(cameraDirRad)};
    sf::Vector2f n_plane{-direction.y, direction.x};
    float focalLength = (m_settings.resolution.x / 2.0f) / m_settings.fovScale;

    sf::Vector2f toEffect = sf::Vector2f(effect.pos.x, effect.pos.y) - camera.cameraPos.posf;
    float signedPerpWorldDistance = dotProduct(toEffect, direction);
    if (signedPerpWorldDistance <= 0) return;

    float bottomScreen = focalLength * (camera.cameraHeight + camera.cameraHeightStart - effect.heightStart) / signedPerpWorldDistance +
                         m_settings.resolution.y / 2.0f;
    float topScreen = focalLength * (camera.cameraHeight + camera.cameraHeightStart - effect.height - effect.heightStart) / signedPerpWorldDistance +
                      m_settings.resolution.y / 2.0f;
    float heightScreen = bottomScreen - topScreen;
    float widthScreen = focalLength * effect.width / signedPerpWorldDistance;
    float screenX = (m_settings.resolution.x / 2.0f) + focalLength * dotProduct(toEffect, n_plane) / signedPerpWorldDistance;

    int tiledW = static_cast<int>(texSize.x / effect.scaleX);
    int tiledH = static_cast<int>(texSize.y / effect.scaleY);

    sf::Sprite sprite(tex);
    sprite.setTextureRect(sf::IntRect({0, 0}, {tiledW, tiledH}));
    sprite.setOrigin({tiledW / 2.0f, tiledH / 2.0f});
    sprite.setPosition({screenX, topScreen + heightScreen / 2.0f});
    sprite.setScale({widthScreen / tiledW, heightScreen / tiledH});

    float shade = std::min(1.0f, m_settings.shaderScale / signedPerpWorldDistance);
    std::uint8_t shadeValue = static_cast<std::uint8_t>(255 * shade);
    sprite.setColor(sf::Color(shadeValue, shadeValue, shadeValue, 255));
    m_renderTexture.draw(sprite);
    ++m_debugSettings.numEffectsRenderedFrame;
}

void TwoHalfD::Renderer::renderFloorSection(const TwoHalfD::FloorSection *floorSection, const CameraObject &camera) {
    float focalLength = (m_settings.resolution.x / 2.0f) / m_settings.fovScale;
    XYVectorf n_direction{std::cos(camera.cameraPos.direction), std::sin(camera.cameraPos.direction)};
    XYVectorf n_plane{-n_direction.y, n_direction.x};
    const float NEAR_CLIP = 1.0f;

    auto texIt = m_textures->find(floorSection->textureId);
    if (texIt == m_textures->end()) {
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
        float p_yScreenPos = (m_settings.resolution.y / 2.0f) +
                             focalLength * (camera.cameraHeight + camera.cameraHeightStart - floorSection->height) / perpWorldDistance;
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
    ++m_debugSettings.numFloorSectionsRenderedFrame;
}

void TwoHalfD::Renderer::renderColourOverlay(const TwoHalfD::FloorColourOverlay *overlay, const CameraObject &camera) {
    float focalLength = (m_settings.resolution.x / 2.0f) / m_settings.fovScale;
    XYVectorf n_direction{std::cos(camera.cameraPos.direction), std::sin(camera.cameraPos.direction)};
    XYVectorf n_plane{-n_direction.y, n_direction.x};
    const float NEAR_CLIP = 100.0f;

    std::vector<XYVectorf> vertices{};
    size_t n = overlay->vertices.size();
    vertices.reserve(n);

    for (size_t i{}; i < n; ++i) {
        const XYVectorf &curr = overlay->vertices[i];
        const XYVectorf &next = overlay->vertices[(i + 1) % n];

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

    if (vertices.size() < 3) return;

    sf::VertexArray floorShape(sf::PrimitiveType::TriangleFan, vertices.size());
    sf::Color colour(overlay->r, overlay->g, overlay->b, overlay->a);
    for (size_t i = 0; i < vertices.size(); ++i) {
        XYVectorf cameraVertexVec = vertices[i] - camera.cameraPos.posf;
        float perpWorldDistance = dotProduct(cameraVertexVec, n_direction);
        float lateralDist = dotProduct(cameraVertexVec, n_plane);
        float p_xScreenPos = (m_settings.resolution.x / 2.0f) + focalLength * lateralDist / perpWorldDistance;
        float p_yScreenPos = (m_settings.resolution.y / 2.0f) + focalLength * (camera.cameraHeight - overlay->height) / perpWorldDistance;
        floorShape[i].position = sf::Vector2f(p_xScreenPos, p_yScreenPos);
        floorShape[i].color = colour;
    }

    sf::RenderStates states;
    states.blendMode = sf::BlendAlpha;
    m_renderTexture.draw(floorShape, states);
    ++m_debugSettings.numColourOverlaysRenderedFrame;
}

void TwoHalfD::Renderer::renderFloor(const CameraObject &camera) {
    float focalLength = (m_settings.resolution.x / 2.0f) / m_settings.fovScale;
    XYVectorf n_direction{std::cos(camera.cameraPos.direction), std::sin(camera.cameraPos.direction)};
    XYVectorf n_plane{-n_direction.y, n_direction.x};

    if (m_defaultFloorTextureId != -1) {
        auto it = m_textures->find(m_defaultFloorTextureId);
        if (it == m_textures->end()) {
            std::cerr << "No texture found for default floor with texture id: " << m_defaultFloorTextureId << std::endl;
            exit(1);
        }
        const sf::Texture &floorTileTexture = it->second.texture;

        sf::VertexArray quad(sf::PrimitiveType::TriangleFan, 4);
        quad[0].position = sf::Vector2f(0, m_settings.resolution.y / 2.0f);
        quad[1].position = sf::Vector2f(0, m_settings.resolution.y);
        quad[2].position = sf::Vector2f(m_settings.resolution.x, m_settings.resolution.y);
        quad[3].position = sf::Vector2f(m_settings.resolution.x, m_settings.resolution.y / 2.0f);

        sf::RenderStates states;
        states.texture = &floorTileTexture;
        states.shader = &m_floorShader;

        m_floorShader.setUniform("textureStartCord", sf::Vector2f(m_defaultFloorStart.x, m_defaultFloorStart.y));
        m_floorShader.setUniform("texture", floorTileTexture);
        m_floorShader.setUniform("textureSize", sf::Vector2f(floorTileTexture.getSize()));
        m_floorShader.setUniform("cameraPos", sf::Vector2f(camera.cameraPos.pos.x, camera.cameraPos.pos.y));
        m_floorShader.setUniform("relativeCameraHeight", camera.cameraHeight + camera.cameraHeightStart - m_defaultFloorHeight);
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
    // static bool loaded = false;
    // static sf::Font font;
    // if (!loaded) {
    //     font.loadFromFile(fs::path(ASSETS_DIR) / "fonts" / "RasterForgeRegular-JpBgm.ttf");
    //     loaded = true;
    // }

    // sf::Text text;
    // text.setFont(font);
    // std::string fpsString = "Fps: " + std::to_string(static_cast<int>(std::round(m_clocks.getAverageGraphicsFps())));
    // text.setString(fpsString);
    // text.setCharacterSize(24);
    // text.setFillColor(sf::Color::Yellow);
    // text.setPosition(m_settings.resolution.x - 120, m_settings.resolution.y - 50);
    // m_renderTexture.draw(text);

    // sf::Text text1;
    // text1.setFont(font);
    // std::string position = "(" + std::to_string(camera.cameraPos.pos.x) + ", " + std::to_string(camera.cameraPos.pos.y) + ")";
    // text1.setString(position);
    // text1.setCharacterSize(24);
    // text1.setFillColor(sf::Color::Yellow);
    // text1.setPosition(50, m_settings.resolution.y - 50);
    // m_renderTexture.draw(text1);
    // renderDebugOverlays(camera, std::nullopt);
}

void TwoHalfD::Renderer::renderDebugOverlays(const TwoHalfD::CameraObject &camera, const std::optional<TwoHalfD::SpriteEntity> &spriteEntity,
                                             const BSPManager &bsp) {
    bool shouldUpdate = m_debugSettings.needsUpdate || m_debugSettings.updateTimer.getElapsedTime().asSeconds() >= m_debugSettings.updateInterval;

    if (shouldUpdate) {
        m_debugSettings.updateTimer.restart();
        m_debugSettings.needsUpdate = false;

        std::string text;
        text += std::format("FPS: {:<5d} SEG: {:<4d} SPR: {:<4d} FLR: {:<4d} CLO: {:<4d} EFC: {:<4d}\n",
                            static_cast<int>(std::round(m_clocks.getAverageGraphicsFps())), m_debugSettings.numSegmentsRenderedFrame,
                            m_debugSettings.numSpritesRenderedFrame, m_debugSettings.numFloorSectionsRenderedFrame,
                            m_debugSettings.numColourOverlaysRenderedFrame, m_debugSettings.numEffectsRenderedFrame);

        text += std::format("CamPos: ({:.2f}, {:.2f}) Dir: {:.2f} Height: {:.2f} Node: {:<3d}\n", camera.cameraPos.pos.x, camera.cameraPos.pos.y,
                            camera.cameraPos.direction, camera.cameraHeight + camera.cameraHeightStart, camera.nodeId);

        std::unordered_set<const TwoHalfD::BSPNode *> uniqueBspRegions;
        uniqueBspRegions.insert(bsp.getLeafNode(camera.nodeId));

        for (size_t i = 0; i < camera.perimeterPoints.size(); ++i) {
            const auto &bp = camera.perimeterPoints[i];
            text += std::format("BP {:<2d}: ({:.2f}, {:.2f}) Node: {:<3d}\n", i, camera.cameraPos.pos.x + bp.offset.x,
                                camera.cameraPos.pos.y + bp.offset.y, bp.bspRegion ? bp.bspRegion->leafNodeId : -1);
            if (bp.bspRegion) uniqueBspRegions.insert(bp.bspRegion);
        }

        text += "\n";

        for (const auto node : uniqueBspRegions) {
            if (!node) continue;
            text += std::format("--- Node {:<3d} ---\n", node->leafNodeId);

            std::string boundsLine = std::format("Bounds ({}): ", node->boundingPolygon.size());
            for (size_t i = 0; i < node->boundingPolygon.size(); ++i) {
                const auto &v = node->boundingPolygon[i];
                boundsLine += std::format("({:.0f},{:.0f}) ", v.x, v.y);
                if (boundsLine.length() > 60) {
                    text += boundsLine + "\n";
                    boundsLine = "  ";
                }
            }
            text += boundsLine + "\n";

            for (size_t i = 0; i < node->boundingSegmentIds.size(); ++i) {
                const auto &seg = bsp.getSegment(node->boundingSegmentIds[i]);
                std::string type = seg.isFloorBoundary() ? "F" : (seg.isWall() ? "W" : "U");
                text += std::format("Seg {:<2d}: ({:.0f},{:.0f})-({:.0f},{:.0f}) {}\n", i, seg.v1.x, seg.v1.y, seg.v2.x, seg.v2.y, type);
            }
        }

        m_debugSettings.cachedText->setFont(m_debugSettings.debugFont);
        m_debugSettings.cachedText->setFillColor(m_debugSettings.debugColor);
        m_debugSettings.cachedText->setCharacterSize(m_debugSettings.fontSize);
        m_debugSettings.cachedText->setPosition({10, 10});
        m_debugSettings.cachedText->setString(text);
    }

    m_renderTexture.draw(*m_debugSettings.cachedText);
}
