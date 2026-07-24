#include "SymmetryManager.h"
#include <cmath>

SymmetryManager::SymmetryManager() : currentMode(SymmetryMode::None), centerPoint(0.f, 0.f), radialSegments(4) {}

void SymmetryManager::setMode(SymmetryMode mode) { currentMode = mode; }
void SymmetryManager::setCenter(sf::Vector2f center) { centerPoint = center; }
void SymmetryManager::setRadialSegments(int segments) { radialSegments = std::max(2, std::min(segments, 32)); }

std::vector<sf::Vector2f> SymmetryManager::getSymmetricPoints(sf::Vector2f point) {
    std::vector<sf::Vector2f> points;
    points.push_back(point);

    if (currentMode == SymmetryMode::Horizontal) {
        points.push_back(sf::Vector2f(centerPoint.x - (point.x - centerPoint.x), point.y));
    }
    else if (currentMode == SymmetryMode::Vertical) {
        points.push_back(sf::Vector2f(point.x, centerPoint.y - (point.y - centerPoint.y)));
    }
    else if (currentMode == SymmetryMode::Both) {
        points.push_back(sf::Vector2f(centerPoint.x - (point.x - centerPoint.x), point.y));
        points.push_back(sf::Vector2f(point.x, centerPoint.y - (point.y - centerPoint.y)));
        points.push_back(sf::Vector2f(centerPoint.x - (point.x - centerPoint.x), centerPoint.y - (point.y - centerPoint.y)));
    }
    else if (currentMode == SymmetryMode::Radial) {
        float dx = point.x - centerPoint.x;
        float dy = point.y - centerPoint.y;
        float radius = std::sqrt(dx * dx + dy * dy);
        float baseAngle = std::atan2(dy, dx);
        float angleStep = (2.0f * 3.14159265f) / radialSegments;

        for (int i = 1; i < radialSegments; ++i) {
            float currentAngle = baseAngle + i * angleStep;
            points.push_back(sf::Vector2f(centerPoint.x + std::cos(currentAngle) * radius, centerPoint.y + std::sin(currentAngle) * radius));
        }
    }
    return points;
}

void SymmetryManager::drawGuides(sf::RenderWindow& window, const sf::RenderStates& states, const sf::FloatRect& drawArea, float scale) {
    if (currentMode == SymmetryMode::None) return;

    sf::VertexArray lines(sf::Lines);
    sf::Color guideColor(0, 255, 255, 128);

    if (currentMode == SymmetryMode::Horizontal || currentMode == SymmetryMode::Both) {
        lines.append(sf::Vertex(sf::Vector2f(centerPoint.x, drawArea.top), guideColor));
        lines.append(sf::Vertex(sf::Vector2f(centerPoint.x, drawArea.top + drawArea.height), guideColor));
    }
    if (currentMode == SymmetryMode::Vertical || currentMode == SymmetryMode::Both) {
        lines.append(sf::Vertex(sf::Vector2f(drawArea.left, centerPoint.y), guideColor));
        lines.append(sf::Vertex(sf::Vector2f(drawArea.left + drawArea.width, centerPoint.y), guideColor));
    }
    if (currentMode == SymmetryMode::Radial) {
        float maxRadius = std::sqrt(drawArea.width * drawArea.width + drawArea.height * drawArea.height);
        float angleStep = (2.0f * 3.14159265f) / radialSegments;
        for (int i = 0; i < radialSegments; ++i) {
            float angle = i * angleStep;
            lines.append(sf::Vertex(centerPoint, guideColor));
            lines.append(sf::Vertex(sf::Vector2f(centerPoint.x + std::cos(angle) * maxRadius, centerPoint.y + std::sin(angle) * maxRadius), guideColor));
        }
    }
    window.draw(lines, states);
}