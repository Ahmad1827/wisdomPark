#include "ToolOptionsBar.h"
#include "../UITheme.h"
#include <algorithm>

namespace WisdomUI {

    ToolOptionsBar::ToolOptionsBar() = default;

    void ToolOptionsBar::Initialize(const sf::Font& font) {
        m_font = font;
    }

    void ToolOptionsBar::SetBounds(const sf::FloatRect& bounds) {
        m_bounds = bounds;
        m_sliderBounds = sf::FloatRect(bounds.left + 190.0f, bounds.top + 10.0f, 110.0f, 12.0f);
        m_perfBtnBounds = sf::FloatRect(bounds.left + 380.0f, bounds.top + 5.0f, 92.0f, 22.0f);
    }

    void ToolOptionsBar::SyncState(const std::string& toolName, float size, bool pixelMode, bool pixelPerfect) {
        m_activeToolName = toolName;
        m_size = size;
        m_pixelMode = pixelMode;
        m_pixelPerfect = pixelPerfect;
    }

    void ToolOptionsBar::Update(float deltaTime, const sf::Vector2f& mousePos) {
        m_globalTime += deltaTime;
        bool perfHover = m_perfBtnBounds.contains(mousePos);
        m_perfHoverAlpha += ((perfHover ? 1.0f : 0.0f) - m_perfHoverAlpha) * 14.0f * deltaTime;

        float targetToggle = m_pixelPerfect ? 1.0f : 0.0f;
        m_perfToggleProgress += (targetToggle - m_perfToggleProgress) * 16.0f * deltaTime;

        bool sliderHover = m_sliderBounds.contains(mousePos) || m_isDraggingSlider;
        m_sliderThumbScale += ((sliderHover ? 1.3f : 1.0f) - m_sliderThumbScale) * 18.0f * deltaTime;
    }

    bool ToolOptionsBar::HandleEvent(const sf::Event& event, const sf::RenderWindow& window,
        std::function<void(float)> onSizeChange,
        std::function<void()> onTogglePixelPerfect) {
        sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            if (m_sliderBounds.contains(mousePos)) {
                m_isDraggingSlider = true;
            }
            else if (m_pixelMode && m_perfBtnBounds.contains(mousePos)) {
                if (onTogglePixelPerfect) onTogglePixelPerfect();
                return true;
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
        sf::RectangleShape bg(sf::Vector2f(m_bounds.width, m_bounds.height));
        bg.setPosition(m_bounds.left, m_bounds.top);
        bg.setFillColor(Theme::Parchment);
        window.draw(bg);

        sf::RectangleShape border(sf::Vector2f(m_bounds.width, 2.0f));
        border.setPosition(m_bounds.left, m_bounds.top + m_bounds.height - 2.0f);
        border.setFillColor(Theme::Brass);
        window.draw(border);

        sf::Text toolLabel("Tool: " + m_activeToolName, m_font, 12);
        toolLabel.setFillColor(Theme::TextParchment);
        toolLabel.setPosition(m_bounds.left + 16.0f, m_bounds.top + 7.0f);
        window.draw(toolLabel);

        sf::Text sizeLabel("Size:", m_font, 12);
        sizeLabel.setFillColor(Theme::TextParchmentMuted);
        sizeLabel.setPosition(m_bounds.left + 150.0f, m_bounds.top + 7.0f);
        window.draw(sizeLabel);

        sf::RectangleShape sliderTrack(sf::Vector2f(m_sliderBounds.width, m_sliderBounds.height));
        sliderTrack.setPosition(m_sliderBounds.left, m_sliderBounds.top);
        sliderTrack.setFillColor(Theme::ParchmentInset);
        sliderTrack.setOutlineThickness(1.0f);
        sliderTrack.setOutlineColor(Theme::Brass);
        window.draw(sliderTrack);

        float maxVal = m_pixelMode ? 32.0f : 100.0f;
        float fillRatio = std::clamp(m_size / maxVal, 0.0f, 1.0f);
        float fillW = fillRatio * m_sliderBounds.width;

        sf::RectangleShape sliderFill(sf::Vector2f(fillW, m_sliderBounds.height));
        sliderFill.setPosition(m_sliderBounds.left, m_sliderBounds.top);
        sliderFill.setFillColor(Theme::Brass);
        window.draw(sliderFill);

        float thumbX = m_sliderBounds.left + fillW;
        float thumbY = m_sliderBounds.top + m_sliderBounds.height / 2.0f;
        sf::RectangleShape thumb(sf::Vector2f(8.0f, 16.0f));
        thumb.setOrigin(4.0f, 8.0f);
        thumb.setPosition(thumbX, thumbY);
        thumb.setScale(m_sliderThumbScale, m_sliderThumbScale);
        thumb.setFillColor(Theme::WoodMedium);
        thumb.setOutlineThickness(1.0f);
        thumb.setOutlineColor(Theme::Gold);
        window.draw(thumb);

        sf::Text sizeVal(std::to_string(static_cast<int>(m_size)) + "px", m_font, 11);
        sizeVal.setFillColor(Theme::TextParchment);
        sizeVal.setPosition(m_sliderBounds.left + m_sliderBounds.width + 12.0f, m_bounds.top + 7.0f);
        window.draw(sizeVal);

        if (m_pixelMode) {
            sf::RectangleShape perfBtn(sf::Vector2f(m_perfBtnBounds.width, m_perfBtnBounds.height));
            perfBtn.setPosition(m_perfBtnBounds.left, m_perfBtnBounds.top);

            sf::Color btnBg = Animation::InterpolateColor(Theme::ParchmentInset, Theme::RubyAccent, m_perfToggleProgress);
            perfBtn.setFillColor(btnBg);
            perfBtn.setOutlineThickness(1.0f);
            perfBtn.setOutlineColor(Animation::InterpolateColor(Theme::Brass, Theme::Gold, m_perfToggleProgress));
            window.draw(perfBtn);

            sf::Text perfText("Pixel Perfect", m_font, 10);
            perfText.setFillColor(Animation::InterpolateColor(Theme::TextParchment, sf::Color::White, m_perfToggleProgress));
            perfText.setPosition(m_perfBtnBounds.left + 12.0f, m_perfBtnBounds.top + 4.0f);
            window.draw(perfText);
        }
    }

}