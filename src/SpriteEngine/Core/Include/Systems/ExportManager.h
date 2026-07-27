#pragma once
#include <SFML/Graphics/Image.hpp>
#include <string>

namespace StudioCore {

class Project;

class ExportManager {
public:
    ExportManager() = default;

    sf::Image GeneratePreview(const Project& project, int padding) const;
    bool ExportPNG(const Project& project, const std::string& filePath, int padding) const;
};

}