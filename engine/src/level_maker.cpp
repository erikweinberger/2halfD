#include "TwoHalfD/level_maker.h"

TwoHalfD::Level TwoHalfD::LevelMaker::parseLevelFile(std::string levelFilePath) {
    std::ifstream inputFile(fs::path(ASSETS_DIR) / levelFilePath);

    TwoHalfD::Level result_level{};

    if (!inputFile.is_open()) {
        std::cerr << "Error opening file!" << std::endl;
        exit(1);
    }

    std::string line;
    while (std::getline(inputFile, line)) {
        size_t spacePos = line.find(' ');
        std::string firstWord;
        if (spacePos != std::string::npos) {
            firstWord = line.substr(0, spacePos);
        } else {
            continue;
        }
        if (firstWord == "#") {
            continue;
        }

        switch (std::stoi(firstWord)) {
        case TwoHalfD::EntityTypes::texture: {
            TwoHalfD::TextureSignature texture = _makeTexture(line);
            m_textures[texture.id] = texture;
            break;
        }
        case TwoHalfD::EntityTypes::wall: {
            m_walls.push_back(_makeWall(line));
            break;
        }
        case TwoHalfD::EntityTypes::sprite: {
            m_spriteEntities.push_back(_makeSpriteEntity(line));
            break;
        }
        case TwoHalfD::EntityTypes::seed: {
            std::istringstream ss(line);
            int skip;
            ss >> skip >> result_level.seed;
            std::cerr << "Level seed set to: " << result_level.seed << '\n';
            break;
        }
        default:
            break;
        }
    }

    result_level.textures = std::move(m_textures);
    result_level.walls = std::move(m_walls);
    result_level.sprites = std::move(m_spriteEntities);
    std::cerr << "Loaded all things lenTex: " << result_level.textures.size() << "len of wall: " << result_level.walls.size()
              << " len of sprites: " << result_level.sprites.size() << '\n';

    return result_level;
}

TwoHalfD::TextureSignature TwoHalfD::LevelMaker::_makeTexture(std::string textureString) {
    std::string word;
    std::stringstream ss(textureString);
    std::string filePath;
    sf::Texture tex;
    int type;
    int textureId{0};

    for (int i = 0; i <= 3 && std::getline(ss, word, ' '); ++i) {
        switch (i) {
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
    if (!tex.loadFromFile(fs::path(ASSETS_DIR) / filePath)) {
        std::cerr << "Incorrect filepath given: (" << filePath << ") will use default\n";
        tex.loadFromFile(m_defaultTextureFilePath);
    }

    switch (type) {
    case TwoHalfD::EntityTypes::wall:
        tex.setRepeated(true);
    case TwoHalfD::EntityTypes::sprite:
        tex.setRepeated(false);
    default:
        break;
    }

    return TwoHalfD::TextureSignature{textureId, filePath, tex};
}

TwoHalfD::Wall TwoHalfD::LevelMaker::_makeWall(std::string wallString) {
    std::string word;
    std::stringstream ss(wallString);
    float startX = 0, startY = 0;
    float endX = 0, endY = 0;
    float height = 0;
    int textureId = 0;

    for (int i = 0; i <= 6 && std::getline(ss, word, ' '); ++i) {
        switch (i) {
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
    if (tex == m_textures.end()) {
        std::cerr << "Texture not loaded (TextureId: " << textureId << ")\n";
    }

    auto wall = TwoHalfD::Wall{m_entityId++, {startX, startY}, {endX, endY}, height, textureId};
    return wall;
}

TwoHalfD::SpriteEntity TwoHalfD::LevelMaker::_makeSpriteEntity(std::string spriteString) {

    float posX, posY;
    int radius;
    int height;
    int textureId;
    float scale = 1.0;

    std::string word;
    std::stringstream ss(spriteString);
    for (int i = 0; i <= 6 && std::getline(ss, word, ' '); ++i) {
        switch (i) {
        case 0:
            break;
        case 1:
            posX = std::stof(word);
            break;
        case 2:
            posY = std::stof(word);
        case 3:
            radius = std::stoi(word);
        case 4:
            height = std::stoi(word);
        case 5:
            textureId = std::stoi(word);
        case 6:
            scale = std::stof(word);
        default:
            break;
        }
    }
    if (m_textures.find(textureId) == m_textures.end()) {
        std::cerr << "No valid texture for spriteEntity at: (" << posX << " , " << posY << ")\n";
    }

    return TwoHalfD::SpriteEntity{m_entityId++, {posX, posY}, radius, height, textureId, scale};
}