#include "StatusBar.h"
#include "../UITheme.h"

namespace WisdomUI {

    StatusBar::StatusBar() = default;

    void StatusBar::Initialize(const sf::Font& font) {
        m_font = font;
    }

    void StatusBar::SetBounds(const sf::FloatRect& bounds) {
        m_bounds = bounds;
    }

    void StatusBar::UpdateData(sf::Vector2u canvasSize, sf::Vector2f cursorCoords, float zoom, int currentLayer, int currentFrame) {
        m_canvasSize = canvasSize;
        m_cursorCoords = cursorCoords;
        m_zoom = zoom;
        m_layer = currentLayer;
        m_frame = currentFrame;
    }

    void StatusBar::Update(float deltaTime) {
        m_globalTime += deltaTime;
    }

    void StatusBar::Render(sf::RenderWindow& window) {
        sf::RectangleShape bg(sf::Vector2f(m_bounds.width, m_bounds.height));
        bg.setPosition(m_bounds.left, m_bounds.top);
        bg.setFillColor(Theme::Panel);
        window.draw(bg);

        sf::RectangleShape border(sf::Vector2f(m_bounds.width, Theme::BorderThickness));
        border.setPosition(m_bounds.left, m_bounds.top);
        border.setFillColor(Theme::Border);
        window.draw(border);

        float jewelPulse = Animation::Pulse(m_globalTime, 3.0f, 0.5f, 1.0f);
        sf::CircleShape jewel(3.0f);
        jewel.setPosition(m_bounds.left + 12.0f, m_bounds.top + 9.0f);
        sf::Color jewelCol = sf::Color(80, 230, 110);
        jewelCol.a = static_cast<sf::Uint8>(255 * jewelPulse);
        jewel.setFillColor(jewelCol);
        window.draw(jewel);

        std::string text = "Park Canvas: " + std::to_string(m_canvasSize.x) + "x" + std::to_string(m_canvasSize.y) +
            "  |  Position: " + std::to_string(static_cast<int>(m_cursorCoords.x)) + ", " + std::to_string(static_cast<int>(m_cursorCoords.y)) +
            "  |  Zoom: " + std::to_string(static_cast<int>(m_zoom * 100.0f)) + "%" +
            "  |  Layer: " + std::to_string(m_layer + 1) +
            "  |  Frame: " + std::to_string(m_frame + 1);

        sf::Text status(text, m_font, 11);
        status.setFillColor(Theme::TextSecondary);
        status.setPosition(m_bounds.left + 26.0f, m_bounds.top + 4.0f);
        window.draw(status);
    }

}