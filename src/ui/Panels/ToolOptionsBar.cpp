#include "ToolOptionsBar.h"
#include "../UITheme.h"
#include <algorithm>
#include <cmath>

namespace WisdomUI {

    ToolOptionsBar::ToolOptionsBar() {
        m_selectionButtons = {
            { "resize", "Resize", sf::FloatRect(), 0.0f },
            { "flip_h", "Flip H", sf::FloatRect(), 0.0f },
            { "flip_v", "Flip V", sf::FloatRect(), 0.0f },
            { "duplicate", "Duplicate", sf::FloatRect(), 0.0f },
            { "deselect", "Deselect", sf::FloatRect(), 0.0f },
            { "delete", "Delete Area", sf::FloatRect(), 0.0f }
        };
    }

    void ToolOptionsBar::Initialize(const sf::Font& font) {
        m_font = font;
    }

    void ToolOptionsBar::updateSelectionButtonLayout() {
        float btnH = 28.0f;
        float btnY = std::floor(m_bounds.top + (m_bounds.height - btnH) * 0.5f);
        float btnX = std::floor(m_bounds.left + 155.0f);
        float spacing = 6.0f;

        for (auto& btn : m_selectionButtons) {
            if (btn.id == "delete") {
                // Space for the Z numeric controls before Delete Area
                btnX += 10.0f;
                m_zLabelBounds = sf::FloatRect(btnX, btnY + 4.0f, 22.0f, 20.0f);
                btnX += 24.0f;

                m_zDecBtnBounds = sf::FloatRect(btnX, btnY, 26.0f, btnH);
                btnX += 26.0f + 3.0f;

                m_zBoxBounds = sf::FloatRect(btnX, btnY, 48.0f, btnH);
                btnX += 48.0f + 3.0f;

                m_zIncBtnBounds = sf::FloatRect(btnX, btnY, 26.0f, btnH);
                btnX += 26.0f + 12.0f;
            }

            float btnW = 74.0f;
            if (btn.id == "resize") btnW = 74.0f;
            else if (btn.id == "flip_h" || btn.id == "flip_v") btnW = 66.0f;
            else if (btn.id == "duplicate") btnW = 90.0f;
            else if (btn.id == "deselect") btnW = 86.0f;
            else if (btn.id == "delete") btnW = 100.0f;

            btn.bounds = sf::FloatRect(btnX, btnY, btnW, btnH);
            btnX += btnW + spacing;
        }
    }

    void ToolOptionsBar::SetBounds(const sf::FloatRect& bounds) {
        m_bounds = bounds;

        float btnH = 28.0f;
        float btnY = std::floor(bounds.top + (bounds.height - btnH) * 0.5f);

        m_sliderBounds = sf::FloatRect(std::floor(bounds.left + 225.0f), std::floor(bounds.top + (bounds.height - 14.0f) * 0.5f), 125.0f, 14.0f);

        m_shapeBtnBounds = sf::FloatRect(std::floor(bounds.left + 410.0f), btnY, 94.0f, btnH);
        m_perfBtnBounds = sf::FloatRect(std::floor(bounds.left + 512.0f), btnY, 115.0f, btnH);

        m_outlineBtnBounds = sf::FloatRect(std::floor(bounds.left + 636.0f), btnY, 82.0f, btnH);
        m_outlineColorBoxBounds = sf::FloatRect(std::floor(bounds.left + 724.0f), btnY, 28.0f, 28.0f);

        m_clearSymBtnBounds = sf::FloatRect(std::floor(bounds.left + bounds.width - 150.0f), btnY, 134.0f, btnH);

        m_stabSliderBounds = sf::FloatRect(std::floor(bounds.left + 475.0f), std::floor(bounds.top + (bounds.height - 14.0f) * 0.5f), 125.0f, 14.0f);

        updateSelectionButtonLayout();
    }

    void ToolOptionsBar::SyncState(const std::string& toolName, float size, bool pixelMode, bool pixelPerfect, float stabilization, int zOrder, int maxZ, PixelBrushShape shape, bool hasSelection, bool symmetryActive) {
        m_activeToolName = toolName;
        m_size = size;
        m_pixelMode = pixelMode;
        m_pixelPerfect = pixelPerfect;
        m_stabilization = stabilization;
        if (!m_isTypingZ) {
            m_currentZOrder = zOrder;
        }
        m_maxZOrder = maxZ;
        m_pixelBrushShape = shape;
        m_hasSelection = hasSelection;
        m_symmetryActive = symmetryActive;
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

            bool colorBoxHover = m_outlineColorBoxBounds.contains(mousePos);
            m_outlineColorBoxHoverAlpha += ((colorBoxHover ? 1.0f : 0.0f) - m_outlineColorBoxHoverAlpha) * 14.0f * deltaTime;

            bool sliderHover = m_sliderBounds.contains(mousePos) || m_isDraggingSlider;
            m_sliderThumbScale += ((sliderHover ? 1.25f : 1.0f) - m_sliderThumbScale) * 18.0f * deltaTime;

            bool stabSliderHover = m_stabSliderBounds.contains(mousePos) || m_isDraggingStabSlider;
            m_stabSliderThumbScale += ((stabSliderHover ? 1.25f : 1.0f) - m_stabSliderThumbScale) * 18.0f * deltaTime;
        }
    }

    bool ToolOptionsBar::HandleEvent(const sf::Event& event, const sf::RenderWindow& window,
        std::function<void(float)> onSizeChange,
        std::function<void()> onTogglePixelPerfect,
        std::function<void(const std::string&)> onSelectAction,
        std::function<void()> onMakeOutline,
        std::function<void()> onPickOutlineColor,
        std::function<void(float)> onStabilizationChange,
        std::function<void(int)> onSetZOrder,
        std::function<void(PixelBrushShape)> onShapeChange,
        std::function<void()> onClearSymmetry) {

        sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            if (m_symmetryActive && m_clearSymBtnBounds.contains(mousePos)) {
                if (onClearSymmetry) onClearSymmetry();
                return true;
            }
        }
        bool isSelectTool = (m_activeToolName == "Select" || m_activeToolName == "Magic Wand");
        bool showStab = !m_pixelMode && (m_activeToolName == "Brush" || m_activeToolName == "Pencil");

        if (isSelectTool && m_isTypingZ) {
            if (event.type == sf::Event::TextEntered) {
                if (event.text.unicode == '\b') {
                    if (!m_zInputBuffer.empty()) m_zInputBuffer.pop_back();
                    return true;
                }
                else if (event.text.unicode >= '0' && event.text.unicode <= '9' && m_zInputBuffer.length() < 4) {
                    m_zInputBuffer += static_cast<char>(event.text.unicode);
                    return true;
                }
            }
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Enter) {
                    if (!m_zInputBuffer.empty() && onSetZOrder) {
                        try {
                            int val = std::stoi(m_zInputBuffer);
                            onSetZOrder(val);
                        }
                        catch (...) {}
                    }
                    m_isTypingZ = false;
                    m_zInputBuffer.clear();
                    return true;
                }
                if (event.key.code == sf::Keyboard::Escape) {
                    m_isTypingZ = false;
                    m_zInputBuffer.clear();
                    return true;
                }
            }
        }

        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            if (isSelectTool && m_hasSelection) {
                if (m_zDecBtnBounds.contains(mousePos)) {
                    if (onSetZOrder) onSetZOrder(m_currentZOrder - 1);
                    return true;
                }
                if (m_zIncBtnBounds.contains(mousePos)) {
                    if (onSetZOrder) onSetZOrder(m_currentZOrder + 1);
                    return true;
                }
                if (m_zBoxBounds.contains(mousePos)) {
                    m_isTypingZ = true;
                    m_zInputBuffer.clear();
                    return true;
                }
                else if (m_isTypingZ) {
                    if (!m_zInputBuffer.empty() && onSetZOrder) {
                        try {
                            int val = std::stoi(m_zInputBuffer);
                            onSetZOrder(val);
                        }
                        catch (...) {}
                    }
                    m_isTypingZ = false;
                    m_zInputBuffer.clear();
                }

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
                    return true;
                }
                else if (showStab && m_stabSliderBounds.contains(mousePos)) {
                    m_isDraggingStabSlider = true;
                    return true;
                }
                else if (m_pixelMode && m_shapeBtnBounds.contains(mousePos)) {
                    if (onShapeChange) {
                        PixelBrushShape nextShape = PixelBrushShape::Square;
                        if (m_pixelBrushShape == PixelBrushShape::Square) nextShape = PixelBrushShape::Circle;
                        else if (m_pixelBrushShape == PixelBrushShape::Circle) nextShape = PixelBrushShape::Slash;
                        else if (m_pixelBrushShape == PixelBrushShape::Slash) nextShape = PixelBrushShape::Rectangle;
                        else if (m_pixelBrushShape == PixelBrushShape::Rectangle) nextShape = PixelBrushShape::Square;
                        onShapeChange(nextShape);
                    }
                    return true;
                }
                else if (m_pixelMode && m_perfBtnBounds.contains(mousePos)) {
                    if (onTogglePixelPerfect) onTogglePixelPerfect();
                    return true;
                }
                else if (m_outlineBtnBounds.contains(mousePos)) {
                    if (onMakeOutline) onMakeOutline();
                    return true;
                }
                else if (m_outlineColorBoxBounds.contains(mousePos)) {
                    if (onPickOutlineColor) onPickOutlineColor();
                    return true;
                }
            }
        }

        if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
            m_isDraggingSlider = false;
            m_isDraggingStabSlider = false;
        }

        if (m_isDraggingSlider && (event.type == sf::Event::MouseMoved || sf::Mouse::isButtonPressed(sf::Mouse::Left))) {
            float ratio = std::clamp((mousePos.x - m_sliderBounds.left) / m_sliderBounds.width, 0.0f, 1.0f);
            float newSize = m_pixelMode ? (1.0f + ratio * 31.0f) : (1.0f + ratio * 99.0f);
            if (onSizeChange) onSizeChange(newSize);
            return true;
        }

        if (m_isDraggingStabSlider && (event.type == sf::Event::MouseMoved || sf::Mouse::isButtonPressed(sf::Mouse::Left))) {
            float ratio = std::clamp((mousePos.x - m_stabSliderBounds.left) / m_stabSliderBounds.width, 0.0f, 1.0f);
            if (onStabilizationChange) onStabilizationChange(ratio);
            return true;
        }

        return m_bounds.contains(mousePos);
    }

    void ToolOptionsBar::Render(sf::RenderWindow& window) {
        Theme::DrawSunsetPanel(window, m_bounds, 1.0f);

        sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
        float barH = m_bounds.height;
        float centerY = std::floor(m_bounds.top + barH * 0.5f);

        Theme::DrawCrispText(window, m_font, "TOOL: " + m_activeToolName, 15, std::floor(m_bounds.left + 24.0f), centerY, Theme::SunsetAmber, sf::Color(14, 6, 20), false, true);

        if (m_activeToolName == "Select" || m_activeToolName == "Magic Wand") {
            if (m_hasSelection) {
                for (const auto& btn : m_selectionButtons) {
                    bool isHov = btn.bounds.contains(mousePos);
                    bool isDel = (btn.id == "delete");
                    Theme::DrawSunsetButton(window, btn.bounds, btn.label, m_font, 12, false, isHov, isDel, 1.0f);
                }

                Theme::DrawCrispText(window, m_font, "Z:", 13, m_zLabelBounds.left, centerY, Theme::TextSecondary, sf::Color(14, 6, 20), false, true);

                bool hovDec = m_zDecBtnBounds.contains(mousePos);
                Theme::DrawSunsetButton(window, m_zDecBtnBounds, "<", m_font, 13, false, hovDec, false, 1.0f);

                sf::RectangleShape zBox(sf::Vector2f(m_zBoxBounds.width, m_zBoxBounds.height));
                zBox.setPosition(m_zBoxBounds.left, m_zBoxBounds.top);
                zBox.setFillColor(Theme::SunsetDeepDark);
                zBox.setOutlineThickness(1.2f);
                zBox.setOutlineColor(m_isTypingZ ? Theme::SunsetGold : Theme::SunsetPlum);
                window.draw(zBox);

                std::string zDisp = m_isTypingZ ? (m_zInputBuffer + "_") : std::to_string(m_currentZOrder);
                Theme::DrawCrispText(window, m_font, zDisp, 14, m_zBoxBounds.left + m_zBoxBounds.width * 0.5f, centerY, Theme::SunsetGold, sf::Color::Transparent, true, true);

                bool hovInc = m_zIncBtnBounds.contains(mousePos);
                Theme::DrawSunsetButton(window, m_zIncBtnBounds, ">", m_font, 13, false, hovInc, false, 1.0f);
            }
            else {
                std::string hint = (m_activeToolName == "Select")
                    ? "Click or drag across canvas to select | Right-Click to deselect"
                    : "Click color region to select | Right-Click to deselect";
                Theme::DrawCrispText(window, m_font, hint, 13, std::floor(m_bounds.left + 160.0f), centerY, Theme::TextSecondary, sf::Color(14, 6, 20), false, true);
            }
        }
        else {
            Theme::DrawCrispText(window, m_font, "SIZE:", 13, std::floor(m_bounds.left + 175.0f), centerY, Theme::TextSecondary, sf::Color(14, 6, 20), false, true);

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
            sf::RectangleShape thumb(sf::Vector2f(10.0f, 20.0f));
            thumb.setOrigin(5.0f, 10.0f);
            thumb.setPosition(std::floor(thumbX), std::floor(thumbY));
            thumb.setScale(m_sliderThumbScale, m_sliderThumbScale);
            thumb.setFillColor(Theme::SunsetPeach);
            thumb.setOutlineThickness(1.0f);
            thumb.setOutlineColor(Theme::SunsetGold);
            window.draw(thumb);

            Theme::DrawCrispText(window, m_font, std::to_string(static_cast<int>(m_size)) + "px", 13, std::floor(m_sliderBounds.left + m_sliderBounds.width + 10.0f), centerY, Theme::TextPrimary, sf::Color(14, 6, 20), false, true);

            bool showStab = !m_pixelMode && (m_activeToolName == "Brush" || m_activeToolName == "Pencil");

            if (showStab) {
                Theme::DrawCrispText(window, m_font, "STAB:", 13, std::floor(m_stabSliderBounds.left - 50.0f), centerY, Theme::TextSecondary, sf::Color(14, 6, 20), false, true);

                sf::RectangleShape stabTrack(sf::Vector2f(m_stabSliderBounds.width, m_stabSliderBounds.height));
                stabTrack.setPosition(m_stabSliderBounds.left, m_stabSliderBounds.top);
                stabTrack.setFillColor(Theme::SunsetDeepDark);
                stabTrack.setOutlineThickness(1.0f);
                stabTrack.setOutlineColor(Theme::SunsetPlum);
                window.draw(stabTrack);

                float stabRatio = std::clamp(m_stabilization, 0.0f, 1.0f);
                float stabFillW = stabRatio * m_stabSliderBounds.width;

                sf::RectangleShape stabFill(sf::Vector2f(stabFillW, m_stabSliderBounds.height));
                stabFill.setPosition(m_stabSliderBounds.left, m_stabSliderBounds.top);
                stabFill.setFillColor(Theme::SunsetGold);
                window.draw(stabFill);

                float stabThumbX = m_stabSliderBounds.left + stabFillW;
                float stabThumbY = m_stabSliderBounds.top + m_stabSliderBounds.height / 2.0f;
                sf::RectangleShape stabThumb(sf::Vector2f(10.0f, 20.0f));
                stabThumb.setOrigin(5.0f, 10.0f);
                stabThumb.setPosition(std::floor(stabThumbX), std::floor(stabThumbY));
                stabThumb.setScale(m_stabSliderThumbScale, m_stabSliderThumbScale);
                stabThumb.setFillColor(Theme::SunsetAmber);
                stabThumb.setOutlineThickness(1.0f);
                stabThumb.setOutlineColor(Theme::SunsetGold);
                window.draw(stabThumb);

                int stabPercent = static_cast<int>(std::round(m_stabilization * 100.0f));
                Theme::DrawCrispText(window, m_font, std::to_string(stabPercent) + "%", 13, std::floor(m_stabSliderBounds.left + m_stabSliderBounds.width + 10.0f), centerY, Theme::TextPrimary, sf::Color(14, 6, 20), false, true);
            }

            if (m_pixelMode) {
                std::string shapeLabel = "[#] Square";
                if (m_pixelBrushShape == PixelBrushShape::Circle) shapeLabel = "(*) Circle";
                else if (m_pixelBrushShape == PixelBrushShape::Slash) shapeLabel = "(/) Slash";
                else if (m_pixelBrushShape == PixelBrushShape::Rectangle) shapeLabel = "(=) Rect";

                bool hovShape = m_shapeBtnBounds.contains(mousePos);
                Theme::DrawSunsetButton(window, m_shapeBtnBounds, shapeLabel, m_font, 12, false, hovShape, false, 1.0f);

                Theme::DrawSunsetButton(window, m_perfBtnBounds, "Pixel Perfect", m_font, 12, m_pixelPerfect, m_perfHoverAlpha > 0.5f, true, 1.0f);
            }

            Theme::DrawSunsetButton(window, m_outlineBtnBounds, "Outline", m_font, 12, false, m_outlineHoverAlpha > 0.5f, false, 1.0f);

            sf::RectangleShape colorBox(sf::Vector2f(m_outlineColorBoxBounds.width, m_outlineColorBoxBounds.height));
            colorBox.setPosition(m_outlineColorBoxBounds.left, m_outlineColorBoxBounds.top);
            colorBox.setFillColor(m_outlineColor);
            colorBox.setOutlineThickness(m_outlineColorBoxHoverAlpha > 0.5f ? 2.0f : 1.2f);
            colorBox.setOutlineColor(m_outlineColorBoxHoverAlpha > 0.5f ? Theme::SunsetGold : Theme::SunsetPlum);
            window.draw(colorBox);

            sf::RectangleShape innerEdge(sf::Vector2f(m_outlineColorBoxBounds.width - 4.0f, m_outlineColorBoxBounds.height - 4.0f));
            innerEdge.setPosition(m_outlineColorBoxBounds.left + 2.0f, m_outlineColorBoxBounds.top + 2.0f);
            innerEdge.setFillColor(sf::Color::Transparent);
            innerEdge.setOutlineThickness(1.0f);
            innerEdge.setOutlineColor(sf::Color(255, 255, 255, 50));
            window.draw(innerEdge);
        }

        if (m_symmetryActive) {
            bool hovSym = m_clearSymBtnBounds.contains(mousePos);
            Theme::DrawSunsetButton(window, m_clearSymBtnBounds, "Clear Symmetry", m_font, 12, false, hovSym, true, 1.0f);
        }
    }

}