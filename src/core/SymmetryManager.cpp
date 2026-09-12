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

std::vector<sf::Vector2f> SymmetryManager::getSymmetricPoints(const sf::Vector2f& point) const {
    if (!enabled || (direction.x == 0.f && direction.y == 0.f)) {
        return { point };
    }

    sf::Vector2f v = endPoint - startPoint;
    float lenSq = v.x * v.x + v.y * v.y;
    if (lenSq < 0.0001f) return { point };

    float t = ((point.x - startPoint.x) * v.x + (point.y - startPoint.y) * v.y) / lenSq;
    if (t < 0.0f || t > 1.0f) {
        return { point };
    }

    sf::Vector2f proj = startPoint + t * v;
    sf::Vector2f reflected = point + 2.0f * (proj - point);
    return { point, reflected };
}

void SymmetryManager::drawGuides(sf::RenderWindow& window, const sf::RenderStates& states, const sf::FloatRect& drawArea, float scale) {
    if (!visible || (direction.x == 0.f && direction.y == 0.f)) return;

    float len = std::hypot(endPoint.x - startPoint.x, endPoint.y - startPoint.y);
    if (len < 1.0f) return;

    float angle = std::atan2(endPoint.y - startPoint.y, endPoint.x - startPoint.x) * 180.f / 3.14159265f;

    // Dark outline behind the line for contrast
    sf::RectangleShape darkLine;
    darkLine.setSize(sf::Vector2f(len, (guideThickness + 2.0f) / scale));
    darkLine.setOrigin(0.f, ((guideThickness + 2.0f) / scale) * 0.5f);
    darkLine.setPosition(startPoint);
    darkLine.setRotation(angle);
    darkLine.setFillColor(sf::Color(14, 6, 20, 220));
    window.draw(darkLine, states);

    // Cyan core line from point to point
    sf::RectangleShape coreLine;
    coreLine.setSize(sf::Vector2f(len, guideThickness / scale));
    coreLine.setOrigin(0.f, (guideThickness / scale) * 0.5f);
    coreLine.setPosition(startPoint);
    coreLine.setRotation(angle);
    coreLine.setFillColor(sf::Color(0, 220, 255, 230));
    window.draw(coreLine, states);

    // Endpoint control handles (grab to lengthen, shorten, or rotate)
    auto drawHandle = [&](sf::Vector2f pos) {
        float r = 7.f / scale;
        sf::CircleShape h(r);
        h.setOrigin(r, r);
        h.setPosition(pos);
        h.setFillColor(sf::Color(255, 215, 60));
        h.setOutlineThickness(2.f / scale);
        h.setOutlineColor(sf::Color(14, 6, 20));
        window.draw(h, states);
        };

    drawHandle(startPoint);
    drawHandle(endPoint);
}