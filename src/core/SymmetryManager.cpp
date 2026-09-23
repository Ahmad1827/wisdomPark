#include "SymmetryManager.h"
#include <cmath>

SymmetryManager::SymmetryManager()
    : startPoint(0.f, 0.f), endPoint(0.f, 0.f), direction(0.f, 0.f), normal(0.f, 0.f),
    enabled(false), visible(false), snapToPixel(false), snapTo45(false),
    guideColor(255, 195, 75, 220), guideThickness(1.5f) {}

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

    // Compute exact screen pixels per logical canvas unit to keep visual sizes constant
    sf::Vector2f p0 = states.transform.transformPoint(0.f, 0.f);
    sf::Vector2f p1 = states.transform.transformPoint(1.f, 0.f);
    float pxPerUnit = std::hypot(p1.x - p0.x, p1.y - p0.y);
    if (pxPerUnit < 0.0001f) pxPerUnit = 1.0f;

    float localLineThick = 1.5f / pxPerUnit;
    float localShadowThick = 3.0f / pxPerUnit;
    float localHandleR = 5.0f / pxPerUnit;
    float localHandleOutline = 1.2f / pxPerUnit;
    float localInnerDotR = 1.5f / pxPerUnit;

    float angle = std::atan2(endPoint.y - startPoint.y, endPoint.x - startPoint.x) * 180.f / 3.14159265f;

    // Contrast shadow under the symmetry axis
    sf::RectangleShape darkLine;
    darkLine.setSize(sf::Vector2f(len, localShadowThick));
    darkLine.setOrigin(0.f, localShadowThick * 0.5f);
    darkLine.setPosition(startPoint);
    darkLine.setRotation(angle);
    darkLine.setFillColor(sf::Color(14, 6, 20, 160));
    window.draw(darkLine, states);

    // Warm amber-gold axis line
    sf::RectangleShape coreLine;
    coreLine.setSize(sf::Vector2f(len, localLineThick));
    coreLine.setOrigin(0.f, localLineThick * 0.5f);
    coreLine.setPosition(startPoint);
    coreLine.setRotation(angle);
    coreLine.setFillColor(sf::Color(255, 195, 75, 220));
    window.draw(coreLine, states);

    // Compact end handles with center pivot points
    auto drawHandle = [&](sf::Vector2f pos) {
        sf::CircleShape h(localHandleR);
        h.setOrigin(localHandleR, localHandleR);
        h.setPosition(pos);
        h.setFillColor(sf::Color(255, 215, 90));
        h.setOutlineThickness(localHandleOutline);
        h.setOutlineColor(sf::Color(14, 6, 20, 230));
        window.draw(h, states);

        sf::CircleShape dot(localInnerDotR);
        dot.setOrigin(localInnerDotR, localInnerDotR);
        dot.setPosition(pos);
        dot.setFillColor(sf::Color(14, 6, 20, 220));
        window.draw(dot, states);
        };

    drawHandle(startPoint);
    drawHandle(endPoint);
}