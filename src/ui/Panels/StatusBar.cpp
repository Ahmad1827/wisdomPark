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

    void StatusBar::Render(sf::RenderWindow& window) {
        sf::RectangleShape bg(sf::Vector2f(m_bounds.width, m_bounds.height));
        bg.setPosition(m_bounds.left, m_bounds.top);
        bg.setFillColor(Theme::Background);
        window.draw(bg);

        sf::RectangleShape border(sf::Vector2f(m_bounds.width, Theme::BorderThickness));
        border.setPosition(m_bounds.left, m_bounds.top);
        border.setFillColor(Theme::Border);
        window.draw(border);

        std::string text = "Canvas: " + std::to_string(m_canvasSize.x) + "x" + std::to_string(m_canvasSize.y) +
            "  |  Cursor: " + std::to_string(static_cast<int>(m_cursorCoords.x)) + ", " + std::to_string(static_cast<int>(m_cursorCoords.y)) +
            "  |  Zoom: " + std::to_string(static_cast<int>(m_zoom * 100.0f)) + "%" +
            "  |  Layer: " + std::to_string(m_layer + 1) +
            "  |  Frame: " + std::to_string(m_frame + 1);

        sf::Text status(text, m_font, 11);
        status.setFillColor(Theme::TextSecondary);
        status.setPosition(m_bounds.left + 16.0f, m_bounds.top + 4.0f);
        window.draw(status);
    }

}