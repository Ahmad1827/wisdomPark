#include "Rendering/CoordinateAxisRenderer.h"
#include <cmath>

namespace StudioUI {

CoordinateAxisRenderer::CoordinateAxisRenderer() = default;

void CoordinateAxisRenderer::Render(sf::RenderWindow& window, const sf::FloatRect& imageBounds, float currentZoom) {
    if (imageBounds.width <= 0 || imageBounds.height <= 0) return;

    sf::VertexArray lines(sf::Lines);

    // X Axis (Red)
    lines.append(sf::Vertex(sf::Vector2f(0, 0), m_xAxisColor));
    lines.append(sf::Vertex(sf::Vector2f(imageBounds.width, 0), m_xAxisColor));

    // Y Axis (Green)
    lines.append(sf::Vertex(sf::Vector2f(0, 0), m_yAxisColor));
    lines.append(sf::Vertex(sf::Vector2f(0, imageBounds.height), m_yAxisColor));

    // Rulers (Tick marks every 100 pixels)
    // Scale tick size inversely to zoom so they remain visible
    float tickSize = 10.0f * currentZoom;

    // X Axis Ticks
    for (float x = 0; x <= imageBounds.width; x += 100.0f) {
        lines.append(sf::Vertex(sf::Vector2f(x, -tickSize), m_rulerColor));
        lines.append(sf::Vertex(sf::Vector2f(x, tickSize), m_rulerColor));
    }

    // Y Axis Ticks
    for (float y = 0; y <= imageBounds.height; y += 100.0f) {
        lines.append(sf::Vertex(sf::Vector2f(-tickSize, y), m_rulerColor));
        lines.append(sf::Vertex(sf::Vector2f(tickSize, y), m_rulerColor));
    }

    // Origin Circle
    sf::CircleShape origin(4.0f * currentZoom);
    origin.setFillColor(sf::Color::White);
    origin.setOrigin(origin.getRadius(), origin.getRadius());
    origin.setPosition(0.0f, 0.0f);

    window.draw(lines);
    window.draw(origin);
}

}