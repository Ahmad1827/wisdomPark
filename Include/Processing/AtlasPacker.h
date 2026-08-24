#pragma once
#include <SFML/Graphics/Image.hpp>
#include <string>
#include <vector>

namespace StudioCore {

class Project;

struct PackedSprite {
    std::string id;
    std::string name;
    int x;
    int y;
    int width;
    int height;
    float pivotX;
    float pivotY;
    float baseline;
};

struct AtlasResult {
    sf::Image image;
    std::vector<PackedSprite> sprites;
};

class AtlasPacker {
public:
    static AtlasResult PackUniformGrid(const Project& project, int padding);
};

}