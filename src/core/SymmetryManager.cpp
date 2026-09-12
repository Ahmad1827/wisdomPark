#include "SymmetryManager.h"
#include <cmath>

SymmetryManager::SymmetryManager()
    : startPoint(0.f, 0.f), endPoint(0.f, 0.f), direction(0.f, 0.f), normal(0.f, 0.f),
    enabled(false), visible(true), snapToPixel(false), snapTo45(false),
    guideColor(0, 255, 255, 200), guideThickness(2.0f) {}

void SymmetryManager::setEndpoints(sf::Vector2f start, sf::Vector2f end) {
    startPoint = start;
    endPoint = end;
    updateVectors();
}

void SymmetryManager::updateVectors() {
    sf::Vector2f diff = endPoint - startPoint;
    float length = std::sqrt(diff.x * diff.x + diff.y * diff.y);
    if (length > 0.0001f) {
        direction = sf::Vector2f(diff.x / length, diff.y / length);
        normal = sf::Vector2f(-direction.y, direction.x);
    }
    else {
        direction = sf::Vector2f(0.f, 0.f);
        normal = sf::Vector2f(0.f, 0.f);
    }
}

std::vector<sf::Vector2f> SymmetryManager::getSymmetricPoints(sf::Vector2f point) {
    std::vector<sf::Vector2f> points;
    points.push_back(point);

    if (!enabled || (direction.x == 0.f && direction.y == 0.f)) {
        return points;
    }

    sf::Vector2f v = point - startPoint;
    float dotProduct = v.x * direction.x + v.y * direction.y;
    sf::Vector2f projection = startPoint + direction * dotProduct;
    sf::Vector2f reflected = point + 2.0f * (projection - point);

    points.push_back(reflected);
    return points;
}

void SymmetryManager::drawGuides(sf::RenderWindow& window, const sf::RenderStates& states, const sf::FloatRect& drawArea, float scale) {
    if (!visible || !enabled || (direction.x == 0.f && direction.y == 0.f)) return;

    float angle = std::atan2(direction.y, direction.x) * 180.f / 3.14159265f;
    float extent = 10000.f;

    sf::RectangleShape darkLine;
    darkLine.setSize(sf::Vector2f(extent * 2.f, (guideThickness + 2.0f) / scale));
    darkLine.setOrigin(extent, ((guideThickness + 2.0f) / scale) * 0.5f);
    darkLine.setPosition(startPoint);
    darkLine.setRotation(angle);
    darkLine.setFillColor(sf::Color(14, 6, 20, 180));
    window.draw(darkLine, states);

    sf::RectangleShape coreLine;
    coreLine.setSize(sf::Vector2f(extent * 2.f, guideThickness / scale));
    coreLine.setOrigin(extent, (guideThickness / scale) * 0.5f);
    coreLine.setPosition(startPoint);
    coreLine.setRotation(angle);
    coreLine.setFillColor(sf::Color(0, 220, 255, 230));
    window.draw(coreLine, states);

    auto drawHandle = [&](sf::Vector2f pos) {
        float r = 6.f / scale;
        sf::CircleShape h(r);
        h.setOrigin(r, r);
        h.setPosition(pos);
        h.setFillColor(sf::Color(255, 215, 60));
        h.setOutlineThickness(1.5f / scale);
        h.setOutlineColor(sf::Color(14, 6, 20));
        window.draw(h, states);
        };

    drawHandle(startPoint);
    drawHandle(endPoint);
}