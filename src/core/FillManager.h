#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

class FillManager {
private:
    bool colorMatches(const sf::Color& target, const sf::Color& test, float tolerance) const;
    void fillScanline(sf::Image& image, int x, int y, const sf::Color& targetColor, const sf::Color& replacementColor, float tolerance);

public:
    FillManager() = default;
    ~FillManager() = default;

    void execute(sf::Image& image, sf::Vector2i startPoint, const sf::Color& targetColor, const sf::Color& replacementColor, float tolerance, bool contiguous);
};