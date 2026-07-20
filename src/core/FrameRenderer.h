#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <array>

class FrameRenderer {
private:
    std::array<sf::Texture, 8> textures;
    std::array<sf::Sprite, 8> sprites;
    bool useFallback;
    float frameThickness;
    sf::Color fallbackColor;

public:
    FrameRenderer();

    void loadTheme(const std::string& themeName);
    void draw(sf::RenderWindow& window, const sf::FloatRect& bounds, const sf::Transform& transform);
};