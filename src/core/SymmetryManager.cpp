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

    sf::Vector2f diff = endPoint - startPoint;
    float length = std::sqrt(diff.x * diff.x + diff.y * diff.y);

    sf::RectangleShape lineShape;
    lineShape.setSize(sf::Vector2f(length, guideThickness / scale));
    lineShape.setOrigin(0.f, (guideThickness / scale) / 2.f);
    lineShape.setPosition(startPoint);

    float angle = std::atan2(diff.y, diff.x) * 180.f / 3.14159265f;
    lineShape.setRotation(angle);
    lineShape.setFillColor(guideColor);

    window.draw(lineShape, states);
}