#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <array>

class FrameRenderer : public sf::Drawable {
private:
    std::array<sf::Texture, 8> textures;
    std::array<sf::Sprite, 8> sprites;
    bool useFallback;
    float frameThickness;
    sf::Color fallbackColor;
    sf::FloatRect innerBounds;

    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

public:
    FrameRenderer();

    // Loads the eight pngs directly from the provided folder path
    void loadTheme(const std::string& directory);

    // Assembles the perfect grid based strictly on the unscaled PNG dimensions
    sf::FloatRect assembleFrame(float canvasWidth, float canvasHeight);
};