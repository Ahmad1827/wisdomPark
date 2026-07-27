#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

class SymmetryManager {
public:
    SymmetryManager();

    void setEndpoints(sf::Vector2f start, sf::Vector2f end);
    void updateVectors();
    std::vector<sf::Vector2f> getSymmetricPoints(sf::Vector2f point);
    void drawGuides(sf::RenderWindow& window, const sf::RenderStates& states, const sf::FloatRect& drawArea, float scale);

    sf::Vector2f startPoint;
    sf::Vector2f endPoint;
    sf::Vector2f direction;
    sf::Vector2f normal;

    bool enabled;
    bool visible;
    bool snapToPixel;
    bool snapTo45;
    sf::Color guideColor;
    float guideThickness;
};