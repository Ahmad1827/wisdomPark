#include "TimelineHeader.h"
#include "../UITheme.h"
#include "../UIIcons.h"

namespace WisdomUI {

    static const unsigned int kTitleSize = 15;
    static const unsigned int kButtonTextSize = 14;

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
        const float btnH = 26.0f;
        float y = std::floor(bounds.top + (bounds.height - btnH) * 0.5f);
        float x = std::floor(bounds.left + 16.0f + Theme::MeasureText(m_font, "TIMELINE", kTitleSize) + 20.0f);

        auto place = [&](sf::FloatRect& target, const std::string& widestLabel, float gapAfter) {
            float w = Theme::MeasureText(m_font, widestLabel, kButtonTextSize) + 24.0f;
            target = sf::FloatRect(x, y, w, btnH);
            x += w + gapAfter;
            };

        place(m_playBtnBounds, "Pause", 14.0f);
        place(m_addBtnBounds, "+ Add", 6.0f);
        place(m_dupBtnBounds, "Duplicate", 6.0f);
        place(m_delBtnBounds, "Delete", 14.0f);
        place(m_onionBtnBounds, "Onion Skin", 0.0f);

        m_closeBtnBounds = sf::FloatRect(std::floor(bounds.left + bounds.width - btnH - 12.0f), y, btnH, btnH);
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
        Theme::DrawCarvedWoodPlank(window, m_bounds, false, 1.0f);

        float centerY = std::floor(m_bounds.top + m_bounds.height * 0.5f);
        const sf::Color textShadow(14, 6, 20);

        Theme::DrawCrispText(window, m_font, "TIMELINE", kTitleSize, m_bounds.left + 16.0f, centerY, Theme::Gold, textShadow, false, true);

        Theme::DrawThemedButton(window, m_playBtnBounds, m_isPlaying ? "Pause" : "Play", m_font, kButtonTextSize, m_isPlaying, m_playHover > 0.5f, m_isPlaying, 1.0f);
        Theme::DrawThemedButton(window, m_addBtnBounds, "+ Add", m_font, kButtonTextSize, false, m_addHover > 0.5f, false, 1.0f);
        Theme::DrawThemedButton(window, m_dupBtnBounds, "Duplicate", m_font, kButtonTextSize, false, m_dupHover > 0.5f, false, 1.0f);
        Theme::DrawThemedButton(window, m_delBtnBounds, "Delete", m_font, kButtonTextSize, false, m_delHover > 0.5f, true, 1.0f);
        Theme::DrawThemedButton(window, m_onionBtnBounds, "Onion Skin", m_font, kButtonTextSize, m_onionEnabled, m_onionHover > 0.5f, false, 1.0f);
        Theme::DrawThemedButton(window, m_closeBtnBounds, "v", m_font, kButtonTextSize, false, m_closeHover > 0.5f, false, 1.0f);

        std::string infoStr = "Frame " + std::to_string(m_currentFrame + 1) + " / " + std::to_string(m_totalFrames) +
            "   |   " + std::to_string(static_cast<int>(m_fps)) + " FPS";
        float infoW = Theme::MeasureText(m_font, infoStr, kButtonTextSize);
        Theme::DrawCrispText(window, m_font, infoStr, kButtonTextSize, m_closeBtnBounds.left - infoW - 16.0f, centerY, Theme::TextSecondary, textShadow, false, true);
    }

}