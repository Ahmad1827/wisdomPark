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
        sf::RectangleShape bg(sf::Vector2f(m_bounds.width, m_bounds.height));
        bg.setPosition(m_bounds.left, m_bounds.top);
        bg.setFillColor(Theme::WoodDark);
        window.draw(bg);

        sf::RectangleShape border(sf::Vector2f(m_bounds.width, 2.0f));
        border.setPosition(m_bounds.left, m_bounds.top);
        border.setFillColor(Theme::Brass);
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

        sf::RectangleShape btn(sf::Vector2f(m_timelineToggleBtn.width, m_timelineToggleBtn.height));
        btn.setPosition(m_timelineToggleBtn.left, m_timelineToggleBtn.top);
        sf::Color btnCol = m_isTimelineOpen ? Theme::RubyAccent : Animation::InterpolateColor(Theme::WoodMedium, Theme::WoodLight, m_toggleHoverAlpha);
        btn.setFillColor(btnCol);
        btn.setOutlineThickness(1.0f);
        btn.setOutlineColor(m_isTimelineOpen ? Theme::Gold : Animation::InterpolateColor(Theme::Brass, Theme::Gold, m_toggleHoverAlpha));
        window.draw(btn);

        std::string btnStr = m_isTimelineOpen ? "[ v ] TIMELINE" : "[ ^ ] TIMELINE";
        sf::Text btnText(btnStr, m_font, 10);
        btnText.setFillColor(m_isTimelineOpen ? sf::Color::White : Animation::InterpolateColor(Theme::TextPrimary, Theme::Gold, m_toggleHoverAlpha));
        sf::FloatRect tb = btnText.getLocalBounds();
        btnText.setPosition(
            m_timelineToggleBtn.left + (m_timelineToggleBtn.width - tb.width) / 2.0f,
            m_timelineToggleBtn.top + 3.0f
        );
        window.draw(btnText);
    }

}