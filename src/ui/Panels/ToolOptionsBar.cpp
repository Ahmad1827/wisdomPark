#include "ToolOptionsBar.h"
#include "../UITheme.h"
#include <algorithm>

namespace WisdomUI {

    ToolOptionsBar::ToolOptionsBar() {
        m_selectionButtons = {
            { "flip_h", "Flip H", sf::FloatRect(), 0.0f },
            { "flip_v", "Flip V", sf::FloatRect(), 0.0f },
            { "duplicate", "Duplicate", sf::FloatRect(), 0.0f },
            { "crop", "Crop Canvas", sf::FloatRect(), 0.0f },
            { "delete", "Delete Area", sf::FloatRect(), 0.0f }
        };
    }

    void ToolOptionsBar::Initialize(const sf::Font& font) {
        m_font = font;
    }

    void ToolOptionsBar::updateSelectionButtonLayout() {
        float btnX = m_bounds.left + 160.0f;
        float btnY = m_bounds.top + 5.0f;
        float btnH = 22.0f;
        float spacing = 6.0f;

        for (auto& btn : m_selectionButtons) {
            float btnW = (btn.id == "crop" || btn.id == "delete") ? 92.0f : ((btn.id == "duplicate") ? 82.0f : 64.0f);
            btn.bounds = sf::FloatRect(btnX, btnY, btnW, btnH);
            btnX += btnW + spacing;
        }
    }

    void ToolOptionsBar::SetBounds(const sf::FloatRect& bounds) {
        m_bounds = bounds;
        m_sliderBounds = sf::FloatRect(bounds.left + 210.0f, bounds.top + 10.0f, 120.0f, 12.0f);
        m_perfBtnBounds = sf::FloatRect(bounds.left + 400.0f, bounds.top + 5.0f, 104.0f, 22.0f);
        m_outlineBtnBounds = sf::FloatRect(bounds.left + 514.0f, bounds.top + 5.0f, 80.0f, 22.0f);
        updateSelectionButtonLayout();
    }

    void ToolOptionsBar::SyncState(const std::string& toolName, float size, bool pixelMode, bool pixelPerfect) {
        m_activeToolName = toolName;
        m_size = size;
        m_pixelMode = pixelMode;
        m_pixelPerfect = pixelPerfect;
    }

    void ToolOptionsBar::Update(float deltaTime, const sf::Vector2f& mousePos) {
        m_globalTime += deltaTime;

        if (m_activeToolName == "Select" || m_activeToolName == "Magic Wand") {
            for (auto& btn : m_selectionButtons) {
                bool hov = btn.bounds.contains(mousePos);
                btn.hoverAlpha += ((hov ? 1.0f : 0.0f) - btn.hoverAlpha) * 16.0f * deltaTime;
            }
        }
        else {
            bool perfHover = m_perfBtnBounds.contains(mousePos);
            m_perfHoverAlpha += ((perfHover ? 1.0f : 0.0f) - m_perfHoverAlpha) * 14.0f * deltaTime;

            float targetToggle = m_pixelPerfect ? 1.0f : 0.0f;
            m_perfToggleProgress += (targetToggle - m_perfToggleProgress) * 16.0f * deltaTime;

            bool outlineHover = m_outlineBtnBounds.contains(mousePos);
            m_outlineHoverAlpha += ((outlineHover ? 1.0f : 0.0f) - m_outlineHoverAlpha) * 14.0f * deltaTime;

            bool sliderHover = m_sliderBounds.contains(mousePos) || m_isDraggingSlider;
            m_sliderThumbScale += ((sliderHover ? 1.25f : 1.0f) - m_sliderThumbScale) * 18.0f * deltaTime;
        }
    }

    bool ToolOptionsBar::HandleEvent(const sf::Event& event, const sf::RenderWindow& window,
        std::function<void(float)> onSizeChange,
        std::function<void()> onTogglePixelPerfect,
        std::function<void(const std::string&)> onSelectAction,
        std::function<void()> onMakeOutline) {

        sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            if (m_activeToolName == "Select" || m_activeToolName == "Magic Wand") {
                for (const auto& btn : m_selectionButtons) {
                    if (btn.bounds.contains(mousePos)) {
                        if (onSelectAction) onSelectAction(btn.id);
                        return true;
                    }
                }
            }
            else {
                if (m_sliderBounds.contains(mousePos)) {
                    m_isDraggingSlider = true;
                }
                else if (m_pixelMode && m_perfBtnBounds.contains(mousePos)) {
                    if (onTogglePixelPerfect) onTogglePixelPerfect();
                    return true;
                }
                else if (m_outlineBtnBounds.contains(mousePos)) {
                    if (onMakeOutline) onMakeOutline();
                    return true;
                }
            }
        }

        if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
            m_isDraggingSlider = false;
        }

        if (m_isDraggingSlider && (event.type == sf::Event::MouseMoved || sf::Mouse::isButtonPressed(sf::Mouse::Left))) {
            float ratio = std::clamp((mousePos.x - m_sliderBounds.left) / m_sliderBounds.width, 0.0f, 1.0f);
            float newSize = m_pixelMode ? (1.0f + ratio * 31.0f) : (1.0f + ratio * 99.0f);
            if (onSizeChange) onSizeChange(newSize);
            return true;
        }

        return m_bounds.contains(mousePos);
    }

    void ToolOptionsBar::Render(sf::RenderWindow& window) {
        Theme::DrawSunsetPanel(window, m_bounds, 1.0f);

        sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

        Theme::DrawCrispText(window, m_font, "TOOL: " + m_activeToolName, 12, m_bounds.left + 24.0f, m_bounds.top + 8.0f, Theme::SunsetAmber);

        if (m_activeToolName == "Select" || m_activeToolName == "Magic Wand") {
            for (const auto& btn : m_selectionButtons) {
                bool isHov = btn.bounds.contains(mousePos);
                bool isDel = (btn.id == "delete");
                Theme::DrawSunsetButton(window, btn.bounds, btn.label, m_font, 10, false, isHov, isDel, 1.0f);
            }
        }
        else {
            Theme::DrawCrispText(window, m_font, "SIZE:", 12, m_bounds.left + 165.0f, m_bounds.top + 8.0f, Theme::TextSecondary);

            sf::RectangleShape sliderTrack(sf::Vector2f(m_sliderBounds.width, m_sliderBounds.height));
            sliderTrack.setPosition(m_sliderBounds.left, m_sliderBounds.top);
            sliderTrack.setFillColor(Theme::SunsetDeepDark);
            sliderTrack.setOutlineThickness(1.0f);
            sliderTrack.setOutlineColor(Theme::SunsetPlum);
            window.draw(sliderTrack);

            float maxVal = m_pixelMode ? 32.0f : 100.0f;
            float fillRatio = std::clamp(m_size / maxVal, 0.0f, 1.0f);
            float fillW = fillRatio * m_sliderBounds.width;

            sf::RectangleShape sliderFill(sf::Vector2f(fillW, m_sliderBounds.height));
            sliderFill.setPosition(m_sliderBounds.left, m_sliderBounds.top);
            sliderFill.setFillColor(Theme::SunsetCoral);
            window.draw(sliderFill);

            float thumbX = m_sliderBounds.left + fillW;
            float thumbY = m_sliderBounds.top + m_sliderBounds.height / 2.0f;
            sf::RectangleShape thumb(sf::Vector2f(8.0f, 16.0f));
            thumb.setOrigin(4.0f, 8.0f);
            thumb.setPosition(std::floor(thumbX), std::floor(thumbY));
            thumb.setScale(m_sliderThumbScale, m_sliderThumbScale);
            thumb.setFillColor(Theme::SunsetPeach);
            thumb.setOutlineThickness(1.0f);
            thumb.setOutlineColor(Theme::SunsetGold);
            window.draw(thumb);

            Theme::DrawCrispText(window, m_font, std::to_string(static_cast<int>(m_size)) + "px", 12, m_sliderBounds.left + m_sliderBounds.width + 12.0f, m_bounds.top + 8.0f, Theme::TextPrimary);

            if (m_pixelMode) {
                Theme::DrawSunsetButton(window, m_perfBtnBounds, "Pixel Perfect", m_font, 11, m_pixelPerfect, m_perfHoverAlpha > 0.5f, true, 1.0f);
            }

            Theme::DrawSunsetButton(window, m_outlineBtnBounds, "Outline", m_font, 11, false, m_outlineHoverAlpha > 0.5f, false, 1.0f);
        }
    }

}