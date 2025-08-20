#include <TwoHalfD/engine.h>
#include "utils/mathUtil.h"

void TwoHalfD::Engine::loadLevel(const Level &level) {
    this->m_engineState = EngineState::running;
    this->m_level = level;
}

void TwoHalfD::Engine::render() {
    sf::Event event;
    while (m_window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            m_window.close();
            m_engineState = EngineState::ended;
        }
    }
    /*
    m_window.clear(sf::Color::Black);

    sf::CircleShape circle(50);
    circle.setFillColor(sf::Color::Green);
    circle.setPosition(100, 100);
    m_window.draw(circle);
    */
    
    this->renderFloorCeil();
    
    m_renderTexture.display();
    sf::Sprite sprite(m_renderTexture.getTexture());
    sprite.setScale(static_cast<float>(m_engineSettings.windowDim.x) / m_engineSettings.resolution.x,
                    static_cast<float>(m_engineSettings.windowDim.y) / m_engineSettings.resolution.y);
    m_window.draw(sprite);
    m_window.display();
}

void TwoHalfD::Engine::renderFloorCeil() {
    static sf::Texture floorTileTexture;
    static sf::Image floorTileImage;
    static bool loaded = false;

    // Update texture loading to own logic/class on level loading temporary
    const char* floor_path{
        "../assets/textures/pattern_18_debug.png"};

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

    float cameraDirRad = degreeToRad(m_cameraPos.direction);
    sf::Vector2f direction{std::cos(cameraDirRad), std::sin(cameraDirRad)};
    sf::Vector2f plane{-direction.y, direction.x * 0.66f};

    float CAMERA_Z = 0.5f * m_engineSettings.resolution.y;
    sf::VertexArray floorVertices(sf::Points);
    floorVertices.resize(m_engineSettings.resolution.x * (m_engineSettings.resolution.y/ 2));
    float textureSize = floorTileTexture.getSize().x;
    if (floorTileTexture.getSize().x != floorTileTexture.getSize().y)
    {
        std::cout << "ERROR NOT SAME SIZE!!!!" << std::endl;
        exit(1);
    }
    for (unsigned int y = m_engineSettings.resolution.y / 2; y < m_engineSettings.resolution.y;
         ++y)
    {
        sf::Vector2f rayDirLeft{direction - plane},
            rayDirRight{direction + plane};
        float rowDistance =
            CAMERA_Z /
            ((float)y - static_cast<float>(m_engineSettings.resolution.y) / 2);
        if (rowDistance > 3.0)
        {
            continue;
        }
        sf::Vector2f floorStep = rowDistance * (rayDirRight - rayDirLeft) /
                                 static_cast<float>(m_engineSettings.resolution.x);

        floorTileImage.getSize();
        sf::Vector2f floor =
            m_cameraPos.pos / textureSize + rowDistance * rayDirLeft;

        for (size_t x = 0; x < m_engineSettings.resolution.x; ++x)
        {
            sf::Vector2i cell{floor};

            sf::Vector2i texCoords{textureSize * (floor - (sf::Vector2f)cell)};

            texCoords.x &= (int)textureSize - 1;
            texCoords.y &= (int)textureSize - 1;

            sf::Color color =
                floorTileImage.getPixel(static_cast<unsigned int>(texCoords.x),
                                        static_cast<unsigned int>(texCoords.y));

            sf::Vertex pixel(sf::Vector2f(x, y), color);
            floorVertices[(y - m_engineSettings.resolution.y / 2) *
                              (m_engineSettings.resolution.x) +
                          x] = pixel;
            floor += floorStep;
        }
    }

    m_renderTexture.draw(floorVertices);
}

TwoHalfD::EngineState TwoHalfD::Engine::getState() {
    return this->m_engineState;
}

