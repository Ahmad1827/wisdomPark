#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <array>

class ColorManager {
private:
    std::vector<sf::Color> recentColors;
    std::vector<sf::Color> customSwatches;
    std::string paletteFilePath;

    void savePalette();
    void loadPalette();

public:
    ColorManager();
    void init();

    void addRecentColor(sf::Color color);
    const std::vector<sf::Color>& getRecentColors() const;

    void addCustomSwatch(sf::Color color);
    void removeCustomSwatch(size_t index);
    const std::vector<sf::Color>& getCustomSwatches() const;

    static sf::Color hsvToRgb(float h, float s, float v);
    static void rgbToHsv(sf::Color c, float& outH, float& outS, float& outV);

    std::array<sf::Color, 1> getComplementary(sf::Color c);
    std::array<sf::Color, 2> getAnalogous(sf::Color c);
    std::array<sf::Color, 2> getTriadic(sf::Color c);
    std::array<sf::Color, 3> getMonochromatic(sf::Color c);
};