#include "StatusBar.h"
#include "../UITheme.h"
#include "../UIIcons.h"

namespace WisdomUI {

    StatusBar::StatusBar() = default;

    void StatusBar::Initialize(const sf::Font& font, std::function<void()> onToggleTimeline) {
        m_font = font;
        m_onToggleTimeline = onToggleTimeline;
    }

    void StatusBar::SetBounds(const sf::FloatRect& bounds) {
        m_bounds = bounds;
        m_timelineToggleBtn = sf::FloatRect(bounds.left + bounds.width - 140.0f, bounds.top + 2.0f, 130.0f, bounds.height - 4.0f);
    }

    void StatusBar::UpdateData(sf::Vector2u canvasSize, sf::Vector2f cursorCoords, float zoom, int currentLayer, int currentFrame, bool isTimelineOpen) {
        m_canvasSize = canvasSize;
        m_cursorCoords = cursorCoords;
        m_zoom = zoom;
        m_layer = currentLayer;
        m_frame = currentFrame;
        m_isTimelineOpen = isTimelineOpen;
    }

    void StatusBar::Update(float deltaTime, const sf::Vector2f& mousePos) {
        m_globalTime += deltaTime;
        bool hover = m_timelineToggleBtn.contains(mousePos);
        m_toggleHoverAlpha += ((hover ? 1.0f : 0.0f) - m_toggleHoverAlpha) * 14.0f * deltaTime;
    }

    bool StatusBar::HandleEvent(const sf::Event& event, const sf::RenderWindow& window) {
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            sf::Vector2f mousePos = window.mapPixelToCoords({ event.mouseButton.x, event.mouseButton.y });
            if (m_timelineToggleBtn.contains(mousePos)) {
                if (m_onToggleTimeline) m_onToggleTimeline();
                return true;
            }
        }
        return false;
    }

    void StatusBar::Render(sf::RenderWindow& window) {
        Theme::DrawCarvedWoodPlank(window, m_bounds, false, 1.0f);

        float jewelPulse = Animation::Pulse(m_globalTime, 3.0f, 0.5f, 1.0f);
        sf::CircleShape jewel(3.5f);
        jewel.setPosition(m_bounds.left + 14.0f, m_bounds.top + 8.5f);
        sf::Color jewelCol = sf::Color(80, 230, 110);
        jewelCol.a = static_cast<sf::Uint8>(255 * jewelPulse);
        jewel.setFillColor(jewelCol);
        jewel.setOutlineThickness(1.0f);
        jewel.setOutlineColor(Theme::Gold);
        window.draw(jewel);

        std::string text = "Park Canvas: " + std::to_string(m_canvasSize.x) + "x" + std::to_string(m_canvasSize.y) +
            "  |  Position: " + std::to_string(static_cast<int>(m_cursorCoords.x)) + ", " + std::to_string(static_cast<int>(m_cursorCoords.y)) +
            "  |  Zoom: " + std::to_string(static_cast<int>(m_zoom * 100.0f)) + "%" +
            "  |  Layer: " + std::to_string(m_layer + 1) +
            "  |  Frame: " + std::to_string(m_frame + 1);

        sf::Text status(text, m_font, 11);
        status.setFillColor(Theme::TextSecondary);
        status.setPosition(m_bounds.left + 28.0f, m_bounds.top + 4.0f);
        window.draw(status);

        std::string btnStr = m_isTimelineOpen ? "[ v ] TIMELINE" : "[ ^ ] TIMELINE";
        Theme::DrawThemedButton(window, m_timelineToggleBtn, btnStr, m_font, 10, m_isTimelineOpen, m_toggleHoverAlpha > 0.5f, m_isTimelineOpen, 1.0f);
    }

}