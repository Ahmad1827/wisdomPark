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
            { "merge", "Merge", sf::FloatRect(), 0.0f },
            { "deselect", "Deselect", sf::FloatRect(), 0.0f },
            { "delete", "Delete Area", sf::FloatRect(), 0.0f }
        };
    }

    void ToolOptionsBar::Initialize(const sf::Font& font) {
        m_font = font;
    }

    static const unsigned int kLabelSize = 14;
    static const unsigned int kButtonTextSize = 13;
    static const float kToolLabelSlot = 190.0f;
    static const float kItemGap = 8.0f;
    static const float kGroupGap = 16.0f;

    static const char* shapeLabel(PixelBrushShape shape) {
        if (shape == PixelBrushShape::Circle) return "(*) Circle";
        if (shape == PixelBrushShape::Slash) return "(/) Slash";
        if (shape == PixelBrushShape::Rectangle) return "(=) Rect";
        return "[#] Square";
    }

    bool ToolOptionsBar::isSelectTool() const {
        return m_activeToolName == "Select" || m_activeToolName == "Magic Wand";
    }

    bool ToolOptionsBar::showStabilizer() const {
        return !m_pixelMode && (m_activeToolName == "Brush" || m_activeToolName == "Pencil");
    }

    // Flows every control left to right from the tool label, so groups never overlap
    // and the hit boxes always match what Render draws.
    void ToolOptionsBar::updateLayout() {
        const float btnH = 30.0f;
        const float sliderW = 140.0f;
        const float sliderH = 14.0f;
        float btnY = std::floor(m_bounds.top + (m_bounds.height - btnH) * 0.5f);
        float sliderY = std::floor(m_bounds.top + (m_bounds.height - sliderH) * 0.5f);

        auto buttonWidth = [&](const std::string& label) {
            return Theme::MeasureText(m_font, label, kButtonTextSize) + 24.0f;
            };

        m_separators.clear();
        m_contentX = std::floor(m_bounds.left + kToolLabelSlot);
        m_separators.push_back(m_contentX - kGroupGap);
        float x = m_contentX;

        auto nextGroup = [&]() {
            x += kGroupGap - kItemGap;
            m_separators.push_back(x);
            x += kGroupGap;
            };

        m_clearSymBtnBounds = sf::FloatRect(std::floor(m_bounds.left + m_bounds.width - buttonWidth("Clear Symmetry") - 16.0f), btnY, buttonWidth("Clear Symmetry"), btnH);

        if (isSelectTool()) {
            for (auto& btn : m_selectionButtons) {
                if (btn.id == "delete") {
                    nextGroup();
                    float zLabelW = Theme::MeasureText(m_font, "Z", kLabelSize);
                    m_zLabelBounds = sf::FloatRect(x, btnY, zLabelW, btnH);
                    x += zLabelW + kItemGap;

                    m_zDecBtnBounds = sf::FloatRect(x, btnY, 26.0f, btnH);
                    x += 26.0f + 4.0f;
                    m_zBoxBounds = sf::FloatRect(x, btnY, 48.0f, btnH);
                    x += 48.0f + 4.0f;
                    m_zIncBtnBounds = sf::FloatRect(x, btnY, 26.0f, btnH);
                    x += 26.0f + kItemGap;
                    nextGroup();
                }

                float btnW = buttonWidth(btn.label);
                btn.bounds = sf::FloatRect(x, btnY, btnW, btnH);
                x += btnW + kItemGap;
            }
            return;
        }

        m_sizeLabelX = x;
        x += Theme::MeasureText(m_font, "SIZE", kLabelSize) + kItemGap + 4.0f;
        m_sliderBounds = sf::FloatRect(x, sliderY, sliderW, sliderH);
        x += sliderW + kItemGap + 4.0f;
        m_sizeValueX = x;
        x += Theme::MeasureText(m_font, "100px", kLabelSize) + kItemGap;

        if (showStabilizer()) {
            nextGroup();
            m_stabLabelX = x;
            x += Theme::MeasureText(m_font, "STAB", kLabelSize) + kItemGap + 4.0f;
            m_stabSliderBounds = sf::FloatRect(x, sliderY, sliderW, sliderH);
            x += sliderW + kItemGap + 4.0f;
            m_stabValueX = x;
            x += Theme::MeasureText(m_font, "100%", kLabelSize) + kItemGap;
        }
        else {
            m_stabSliderBounds = sf::FloatRect();
        }

        if (m_pixelMode) {
            nextGroup();
            // Sized for the widest shape name so the neighbours don't shift when cycling.
            float shapeW = 0.0f;
            for (PixelBrushShape s : { PixelBrushShape::Square, PixelBrushShape::Circle, PixelBrushShape::Slash, PixelBrushShape::Rectangle }) {
                shapeW = std::max(shapeW, buttonWidth(shapeLabel(s)));
            }
            m_shapeBtnBounds = sf::FloatRect(x, btnY, shapeW, btnH);
            x += shapeW + kItemGap;

            float perfW = buttonWidth("Pixel Perfect");
            m_perfBtnBounds = sf::FloatRect(x, btnY, perfW, btnH);
            x += perfW + kItemGap;
        }
        else {
            m_shapeBtnBounds = sf::FloatRect();
            m_perfBtnBounds = sf::FloatRect();
        }

        nextGroup();
        float outlineW = buttonWidth("Outline");
        m_outlineBtnBounds = sf::FloatRect(x, btnY, outlineW, btnH);
        x += outlineW + kItemGap;
        m_outlineColorBoxBounds = sf::FloatRect(x, btnY, btnH, btnH);
    }

    void ToolOptionsBar::SetBounds(const sf::FloatRect& bounds) {
        m_bounds = bounds;
        updateLayout();
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
        updateLayout();
    }

    void ToolOptionsBar::Update(float deltaTime, const sf::Vector2f& mousePos) {
        m_globalTime += deltaTime;

        if (isSelectTool()) {
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
        bool isSelectTool = this->isSelectTool();
        bool showStab = showStabilizer();

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

        const sf::Color textShadow(14, 6, 20);

        Theme::DrawCrispText(window, m_font, m_activeToolName, 15, std::floor(m_bounds.left + 24.0f), centerY, Theme::SunsetAmber, textShadow, false, true);

        bool selectTool = isSelectTool();
        // Without a selection only the hint is shown, so only the first divider applies.
        size_t separatorCount = (selectTool && !m_hasSelection) ? 1 : m_separators.size();
        for (size_t i = 0; i < separatorCount; ++i) {
            Theme::DrawSeparator(window, m_separators[i], centerY, 20.0f);
        }

        if (selectTool) {
            if (m_hasSelection) {
                for (const auto& btn : m_selectionButtons) {
                    bool isHov = btn.bounds.contains(mousePos);
                    bool isDel = (btn.id == "delete");
                    Theme::DrawSunsetButton(window, btn.bounds, btn.label, m_font, kButtonTextSize, false, isHov, isDel, 1.0f);
                }

                Theme::DrawCrispText(window, m_font, "Z", kLabelSize, m_zLabelBounds.left, centerY, Theme::TextSecondary, textShadow, false, true);

                bool hovDec = m_zDecBtnBounds.contains(mousePos);
                Theme::DrawSunsetButton(window, m_zDecBtnBounds, "<", m_font, kButtonTextSize, false, hovDec, false, 1.0f);

                sf::RectangleShape zBox(sf::Vector2f(m_zBoxBounds.width, m_zBoxBounds.height));
                zBox.setPosition(m_zBoxBounds.left, m_zBoxBounds.top);
                zBox.setFillColor(Theme::SunsetDeepDark);
                zBox.setOutlineThickness(1.2f);
                zBox.setOutlineColor(m_isTypingZ ? Theme::SunsetGold : Theme::SunsetPlum);
                window.draw(zBox);

                std::string zDisp = m_isTypingZ ? (m_zInputBuffer + "_") : std::to_string(m_currentZOrder);
                Theme::DrawCrispText(window, m_font, zDisp, 14, m_zBoxBounds.left + m_zBoxBounds.width * 0.5f, centerY, Theme::SunsetGold, sf::Color::Transparent, true, true);

                bool hovInc = m_zIncBtnBounds.contains(mousePos);
                Theme::DrawSunsetButton(window, m_zIncBtnBounds, ">", m_font, kButtonTextSize, false, hovInc, false, 1.0f);
            }
            else {
                std::string hint = (m_activeToolName == "Select")
                    ? "Click or drag across canvas to select | Right-Click to deselect"
                    : "Click color region to select | Right-Click to deselect";
                Theme::DrawCrispText(window, m_font, hint, kLabelSize, m_contentX, centerY, Theme::TextSecondary, textShadow, false, true);
            }
        }
        else {
            Theme::DrawCrispText(window, m_font, "SIZE", kLabelSize, m_sizeLabelX, centerY, Theme::TextSecondary, textShadow, false, true);

            float maxVal = m_pixelMode ? 32.0f : 100.0f;
            drawSlider(window, m_sliderBounds, m_size / maxVal, Theme::SunsetCoral, Theme::SunsetPeach, m_sliderThumbScale);
            Theme::DrawCrispText(window, m_font, std::to_string(static_cast<int>(m_size)) + "px", kLabelSize, m_sizeValueX, centerY, Theme::TextPrimary, textShadow, false, true);

            if (showStabilizer()) {
                Theme::DrawCrispText(window, m_font, "STAB", kLabelSize, m_stabLabelX, centerY, Theme::TextSecondary, textShadow, false, true);

                drawSlider(window, m_stabSliderBounds, m_stabilization, Theme::SunsetGold, Theme::SunsetAmber, m_stabSliderThumbScale);

                int stabPercent = static_cast<int>(std::round(m_stabilization * 100.0f));
                Theme::DrawCrispText(window, m_font, std::to_string(stabPercent) + "%", kLabelSize, m_stabValueX, centerY, Theme::TextPrimary, textShadow, false, true);
            }

            if (m_pixelMode) {
                bool hovShape = m_shapeBtnBounds.contains(mousePos);
                Theme::DrawSunsetButton(window, m_shapeBtnBounds, shapeLabel(m_pixelBrushShape), m_font, kButtonTextSize, false, hovShape, false, 1.0f);

                Theme::DrawSunsetButton(window, m_perfBtnBounds, "Pixel Perfect", m_font, kButtonTextSize, m_pixelPerfect, m_perfHoverAlpha > 0.5f, true, 1.0f);
            }

            Theme::DrawSunsetButton(window, m_outlineBtnBounds, "Outline", m_font, kButtonTextSize, false, m_outlineHoverAlpha > 0.5f, false, 1.0f);

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
            Theme::DrawSunsetButton(window, m_clearSymBtnBounds, "Clear Symmetry", m_font, kButtonTextSize, false, hovSym, true, 1.0f);
        }
    }

    void ToolOptionsBar::drawSlider(sf::RenderWindow& window, const sf::FloatRect& track, float ratio, sf::Color fillColor, sf::Color thumbColor, float thumbScale) {
        sf::RectangleShape trackShape(sf::Vector2f(track.width, track.height));
        trackShape.setPosition(track.left, track.top);
        trackShape.setFillColor(Theme::SunsetDeepDark);
        trackShape.setOutlineThickness(1.0f);
        trackShape.setOutlineColor(Theme::SunsetPlum);
        window.draw(trackShape);

        float fillW = std::clamp(ratio, 0.0f, 1.0f) * track.width;

        sf::RectangleShape fill(sf::Vector2f(fillW, track.height));
        fill.setPosition(track.left, track.top);
        fill.setFillColor(fillColor);
        window.draw(fill);

        sf::RectangleShape thumb(sf::Vector2f(10.0f, 20.0f));
        thumb.setOrigin(5.0f, 10.0f);
        thumb.setPosition(std::floor(track.left + fillW), std::floor(track.top + track.height / 2.0f));
        thumb.setScale(thumbScale, thumbScale);
        thumb.setFillColor(thumbColor);
        thumb.setOutlineThickness(1.0f);
        thumb.setOutlineColor(Theme::SunsetGold);
        window.draw(thumb);
    }

}