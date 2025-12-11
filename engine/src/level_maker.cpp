#include "TwoHalfD/level_maker.h"
#include "TwoHalfD/engine_types.h"

#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Vector2.hpp>
#include <fstream>
#include <string>
#include <utility>

TwoHalfD::Level TwoHalfD::LevelMaker::parseLevelFile(std::string levelFilePath)
{
    std::ifstream inputFile(levelFilePath);

    TwoHalfD::Level result_level{};

    if (!inputFile.is_open())
    {
        std::cerr << "Error opening file!" << std::endl;
        exit(1);
    }

    std::string line;
    while (std::getline(inputFile, line))
    {
        size_t spacePos = line.find(' ');
        std::string firstWord;
        if (spacePos != std::string::npos)
        {
            firstWord = line.substr(0, spacePos);
        }
        else
        {
            continue;
        }

        switch (std::stoi(firstWord))
        {
        case TwoHalfD::EntityTypes::texture:
        {
            TwoHalfD::TextureSignature texture = _makeTexture(line);
            m_textures[texture.id] = texture;
            break;
        }
        case TwoHalfD::EntityTypes::wall:
        {
            m_walls.push_back(_makeWall(line));
            break;
        }
        case TwoHalfD::EntityTypes::sprite:
        {
            break;
        }
        default:
            break;
        }
    }

    result_level.textures = std::move(m_textures);
    result_level.walls = std::move(m_walls);
    result_level.sprites = std::move(m_spriteEntities);
    std::cerr << "Loaded all things lenTex: " << result_level.textures.size() << " len of wall: " << result_level.walls.size()
              << " len of sprites: " << result_level.sprites.size() << '\n';

    return result_level;
}

TwoHalfD::TextureSignature TwoHalfD::LevelMaker::_makeTexture(std::string textureString)
{
    std::string word;
    std::stringstream ss(textureString);
    std::string filePath;
    sf::Texture tex;
    int type;
    int textureId{0};

    for (int i = 0; i <= 3 && std::getline(ss, word, ' '); ++i)
    {
        switch (i)
        {
        case 0:
            break;
        case 1:
            textureId = std::stoi(word);
            break;
        case 2:
            filePath = word;
            break;
        case 3:
            type = std::stoi(word);
            break;
        default:
            break;
        }
    }
    if (!tex.loadFromFile(filePath))
    {
        std::cerr << "Incorrect filepath given: (" << filePath << ") will use default\n";
        tex.loadFromFile(m_defaultTextureFilePath);
    }

    switch (type)
    {
    case TwoHalfD::EntityTypes::wall:
        tex.setRepeated(true);
    case TwoHalfD::EntityTypes::sprite:
        tex.setRepeated(false);
    default:
        break;
    }

    return TwoHalfD::TextureSignature{textureId, filePath, tex};
}

TwoHalfD::Wall TwoHalfD::LevelMaker::_makeWall(std::string wallString)
{
    std::string word;
    std::stringstream ss(wallString);
    float startX = 0, startY = 0;
    float endX = 0, endY = 0;
    float height = 0;
    int textureId = 0;

    for (int i = 0; i <= 6 && std::getline(ss, word, ' '); ++i)
    {
        switch (i)
        {
        case 0:
            break;
        case 1:
            startX = std::stof(word);
            break;
        case 2:
            startY = std::stof(word);
            break;
        case 3:
            endX = std::stof(word);
            break;
        case 4:
            endY = std::stof(word);
            break;
        case 5:
            height = std::stof(word);
            break;
        case 6:
            textureId = std::stoi(word);
            break;
        default:
            break;
        }
    }
    auto tex = m_textures.find(textureId);
    if (tex == m_textures.end())
    {
        std::cerr << "Texture not loaded (TextureId: " << textureId << ")\n";
    }

    return TwoHalfD::Wall{m_entityId++, {startX, startY}, {endX, endY}, height, textureId};
}