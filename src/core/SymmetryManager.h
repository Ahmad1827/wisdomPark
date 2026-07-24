#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

enum class SymmetryMode { None, Horizontal, Vertical, Both, Radial };

class SymmetryManager {
public:
    SymmetryManager();
    void setMode(SymmetryMode mode);
    void setCenter(sf::Vector2f center);
    void setRadialSegments(int segments);
    std::vector<sf::Vector2f> getSymmetricPoints(sf::Vector2f point);
    void drawGuides(sf::RenderWindow& window, const sf::RenderStates& states, const sf::FloatRect& drawArea, float scale);

private:
    SymmetryMode currentMode;
    sf::Vector2f centerPoint;
    int radialSegments;
};