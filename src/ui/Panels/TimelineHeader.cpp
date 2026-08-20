#include "TimelineHeader.h"
#include "../UITheme.h"
#include "../UIIcons.h"

namespace WisdomUI {

    TimelineHeader::TimelineHeader() = default;

    void TimelineHeader::Initialize(const sf::Font& font,
        std::function<void()> onTogglePlay,
        std::function<void()> onAddFrame,
        std::function<void()> onDuplicateFrame,
        std::function<void()> onDeleteFrame,
        std::function<void()> onToggleOnion,
        std::function<void()> onCloseTimeline) {
        m_font = font;
        m_onTogglePlay = onTogglePlay;
        m_onAddFrame = onAddFrame;
        m_onDuplicateFrame = onDuplicateFrame;
        m_onDeleteFrame = onDeleteFrame;
        m_onToggleOnion = onToggleOnion;
        m_onCloseTimeline = onCloseTimeline;
    }

    void TimelineHeader::SetBounds(const sf::FloatRect& bounds) {
        m_bounds = bounds;
        float y = bounds.top + 3.0f;
        m_playBtnBounds = sf::FloatRect(bounds.left + 120.0f, y, 65.0f, 22.0f);
        m_addBtnBounds = sf::FloatRect(bounds.left + 192.0f, y, 55.0f, 22.0f);
        m_dupBtnBounds = sf::FloatRect(bounds.left + 252.0f, y, 55.0f, 22.0f);
        m_delBtnBounds = sf::FloatRect(bounds.left + 312.0f, y, 55.0f, 22.0f);
        m_onionBtnBounds = sf::FloatRect(bounds.left + 372.0f, y, 75.0f, 22.0f);
        m_closeBtnBounds = sf::FloatRect(bounds.left + bounds.width - 32.0f, y, 22.0f, 22.0f);
    }

    void TimelineHeader::SyncState(bool isPlaying, int currentFrame, int totalFrames, float fps, bool onionEnabled) {
        m_isPlaying = isPlaying;
        m_currentFrame = currentFrame;
        m_totalFrames = totalFrames;
        m_fps = fps;
        m_onionEnabled = onionEnabled;
    }

    void TimelineHeader::Update(float deltaTime, const sf::Vector2f& mousePos) {
        auto updateHov = [&](sf::FloatRect b, float& hov) {
            bool isH = b.contains(mousePos);
            hov += ((isH ? 1.0f : 0.0f) - hov) * 14.0f * deltaTime;
            };
        updateHov(m_playBtnBounds, m_playHover);
        updateHov(m_addBtnBounds, m_addHover);
        updateHov(m_dupBtnBounds, m_dupHover);
        updateHov(m_delBtnBounds, m_delHover);
        updateHov(m_onionBtnBounds, m_onionHover);
        updateHov(m_closeBtnBounds, m_closeHover);
    }

    bool TimelineHeader::HandleEvent(const sf::Event& event, const sf::RenderWindow& window) {
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            sf::Vector2f mousePos = window.mapPixelToCoords({ event.mouseButton.x, event.mouseButton.y });

            if (m_playBtnBounds.contains(mousePos)) { if (m_onTogglePlay) m_onTogglePlay(); return true; }
            if (m_addBtnBounds.contains(mousePos)) { if (m_onAddFrame) m_onAddFrame(); return true; }
            if (m_dupBtnBounds.contains(mousePos)) { if (m_onDuplicateFrame) m_onDuplicateFrame(); return true; }
            if (m_delBtnBounds.contains(mousePos)) { if (m_onDeleteFrame) m_onDeleteFrame(); return true; }
            if (m_onionBtnBounds.contains(mousePos)) { if (m_onToggleOnion) m_onToggleOnion(); return true; }
            if (m_closeBtnBounds.contains(mousePos)) { if (m_onCloseTimeline) m_onCloseTimeline(); return true; }

            if (m_bounds.contains(mousePos)) return true;
        }
        return false;
    }

    void TimelineHeader::Render(sf::RenderWindow& window) {
        sf::RectangleShape bg(sf::Vector2f(m_bounds.width, m_bounds.height));
        bg.setPosition(m_bounds.left, m_bounds.top);
        bg.setFillColor(Theme::WoodDark);
        window.draw(bg);

        sf::RectangleShape border(sf::Vector2f(m_bounds.width, 2.0f));
        border.setPosition(m_bounds.left, m_bounds.top);
        border.setFillColor(Theme::Brass);
        window.draw(border);

        sf::Text label("TIMELINE", m_font, 12);
        label.setFillColor(Theme::Gold);
        label.setPosition(m_bounds.left + 14.0f, m_bounds.top + 6.0f);
        window.draw(label);

        auto drawBtn = [&](sf::FloatRect b, const std::string& str, float hov, bool active = false) {
            sf::RectangleShape r(sf::Vector2f(b.width, b.height));
            r.setPosition(b.left, b.top);
            sf::Color fill = active ? Theme::RubyAccent : Animation::InterpolateColor(Theme::WoodMedium, Theme::WoodLight, hov);
            r.setFillColor(fill);
            r.setOutlineThickness(1.0f);
            r.setOutlineColor(active ? Theme::Gold : Animation::InterpolateColor(Theme::Brass, Theme::Gold, hov));
            window.draw(r);

            sf::Text t(str, m_font, 11);
            t.setFillColor(active ? sf::Color::White : Animation::InterpolateColor(Theme::TextPrimary, Theme::Gold, hov));
            sf::FloatRect tb = t.getLocalBounds();
            t.setPosition(b.left + (b.width - tb.width) / 2.0f, b.top + 3.0f);
            window.draw(t);
            };

        drawBtn(m_playBtnBounds, m_isPlaying ? "Pause" : "Play", m_playHover, m_isPlaying);
        drawBtn(m_addBtnBounds, "+ Add", m_addHover);
        drawBtn(m_dupBtnBounds, "Duplicate", m_dupHover);
        drawBtn(m_delBtnBounds, "Delete", m_delHover);
        drawBtn(m_onionBtnBounds, "Onion Skin", m_onionHover, m_onionEnabled);
        drawBtn(m_closeBtnBounds, "v", m_closeHover);

        std::string infoStr = "Frame: " + std::to_string(m_currentFrame + 1) + " / " + std::to_string(m_totalFrames) +
            "  (" + std::to_string(static_cast<int>(m_fps)) + " FPS)";
        sf::Text info(infoStr, m_font, 11);
        info.setFillColor(Theme::TextSecondary);
        info.setPosition(m_bounds.left + m_bounds.width - info.getLocalBounds().width - 45.0f, m_bounds.top + 6.0f);
        window.draw(info);
    }

}