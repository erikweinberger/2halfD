#include <TwoHalfD/engine.h>
#include "TwoHalfD/engine_types.h"
#include "utils/mathUtil.h"

void TwoHalfD::Engine::loadLevel(const Level &level) {
    this->m_engineState = EngineState::fpsState;
    this->m_level = level;
    m_window.setMouseCursorVisible(false);
}

//Game Inputs
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
                std::cerr << "Mouse pos is: " << mouseWinPos.x << " , " << mouseWinPos.y << '\n';
                m_inputArray[m_currentInput] = TwoHalfD::Event::KeyReleased(event.key.code, mouseWinPos.x, mouseWinPos.y);
                ++m_currentInput;
                break;
            }
            case sf::Event::MouseMoved: {
                XYVector mouseDelta = m_engineContext.currentMousePosition - XYVector{ event.mouseMove.x, event.mouseMove.y };
                m_engineContext.prevMousePosition = m_engineContext.currentMousePosition;
                m_engineContext.currentMousePosition = { event.mouseMove.x, event.mouseMove.y };
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

void TwoHalfD::Engine::clearFrameInputs() {
    m_currentInput = 0;
}

void TwoHalfD::Engine::backgroundFrameUpdates() {
    //Calculate current context
    if (m_engineState == TwoHalfD::EngineState::fpsState) {
        XYVector middleScreen = { m_engineSettings.windowDim.x / 2, m_engineSettings.windowDim.y / 2 };
        sf::Vector2i mouseWinPos = sf::Mouse::getPosition(m_window);
        XYVector mousePosition = { mouseWinPos.x, mouseWinPos.y };
        m_engineContext.MouseDelta = mousePosition - middleScreen;
        m_engineContext.currentMousePosition = middleScreen;
        m_engineContext.prevMousePosition = mousePosition;
    }


    //Reset stats for next iteration
    if (m_engineState == TwoHalfD::EngineState::fpsState) {
        sf::Mouse::setPosition({ m_engineContext.currentMousePosition.x, m_engineContext.currentMousePosition.y }, m_window);
    }
}


TwoHalfD::XYVector TwoHalfD::Engine::getMouseDeltaFrame() {
    return m_engineContext.MouseDelta;
}

//Getters and setters
TwoHalfD::WindowDim TwoHalfD::Engine::getWindowDimension() {
    return { m_engineSettings.windowDim.x, m_engineSettings.windowDim.y };
}


void TwoHalfD::Engine::setCameraPosition(const TwoHalfD::Position &newPos) {
    m_cameraPos = newPos;
}

void TwoHalfD::Engine::updateCameraPosition(const TwoHalfD::Position &posUpdate) {
    m_cameraPos += posUpdate;
}

//Render Logic
void TwoHalfD::Engine::render() {
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

    float cameraDirRad = m_cameraPos.direction;
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
            m_cameraPos.posf / textureSize + rowDistance * rayDirLeft;

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
