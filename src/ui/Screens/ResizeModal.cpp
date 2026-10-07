// In-editor resize canvas modal. Drawing and click handling share the layout helpers below.
#include "../UIManager.h"
#include "../UITheme.h"
#include "MenuWidgets.h"
#include <algorithm>
#include <cmath>
#include <string>

using WisdomUI::Theme;
using WisdomUI::Animation;
using namespace MenuWidgets;

namespace {

    const int kMinSize = 8;
    const int kMaxSize = 8192;
    const size_t kMaxDigits = 5;

    sf::FloatRect panelBounds() {
        const float w = 600.0f;
        const float h = 466.0f;
        return sf::FloatRect(std::floor((1920.0f - w) * 0.5f), std::floor((1080.0f - h) * 0.5f), w, h);
    }

    sf::FloatRect widthBox() { sf::FloatRect p = panelBounds(); return sf::FloatRect(p.left + 36.0f, p.top + 150.0f, 250.0f, 48.0f); }
    sf::FloatRect heightBox() { sf::FloatRect p = panelBounds(); return sf::FloatRect(p.left + 314.0f, p.top + 150.0f, 250.0f, 48.0f); }
    sf::FloatRect compareCard() { sf::FloatRect p = panelBounds(); return sf::FloatRect(p.left + 36.0f, p.top + 222.0f, 528.0f, 108.0f); }
    sf::FloatRect applyButton() { sf::FloatRect p = panelBounds(); return sf::FloatRect(p.left + 36.0f, p.top + 352.0f, 254.0f, 52.0f); }
    sf::FloatRect cancelButton() { sf::FloatRect p = panelBounds(); return sf::FloatRect(p.left + 310.0f, p.top + 352.0f, 254.0f, 52.0f); }

    // The buffers only ever hold up to five digits, so this cannot overflow.
    int parseSize(const std::string& text) {
        return text.empty() ? 0 : std::stoi(text);
    }

    bool isValidSize(int value) {
        return value >= kMinSize && value <= kMaxSize;
    }

    // The modal has no open() call, so its entrance is timed from the first frame it is drawn again.
    unsigned int s_lastDrawnFrame = 0;
    float s_openTime = 0.0f;

}

void UIManager::drawResizeModal(sf::RenderWindow& window, Canvas& canvas) {
    if (Theme::s_frame - s_lastDrawnFrame > 1) s_openTime = Theme::s_time;
    s_lastDrawnFrame = Theme::s_frame;

    float elapsed = Theme::s_time - s_openTime;
    float appear = Animation::EaseOutCubic(elapsed / 0.22f);

    sf::View savedView = beginModal(window, appear);

    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    bool mouseDown = sf::Mouse::isButtonPressed(sf::Mouse::Left);

    sf::FloatRect panel = panelBounds();
    const float left = panel.left + 36.0f;
    const float contentW = panel.width - 72.0f;

    sf::Vector2u current = canvas.getCanvasSize();
    int newW = parseSize(m_resizeWBuf);
    int newH = parseSize(m_resizeHBuf);
    bool valid = isValidSize(newW) && isValidSize(newH);

    Theme::DrawSunsetPanel(window, panel, appear);

    // ---- Header ----
    Theme::DrawCrispText(window, font, "RESIZE CANVAS", 30, left, panel.top + 46.0f, Theme::SunsetGold, Theme::SunsetCoralDark, false, true);
    Theme::DrawCrispText(window, font, "SET A NEW CANVAS RESOLUTION", 16, left + 2.0f, panel.top + 78.0f, Theme::SunsetPeach, kTextShadow, false, true);

    sf::RectangleShape rule(sf::Vector2f(contentW, 1.0f));
    rule.setPosition(left, panel.top + 102.0f);
    rule.setFillColor(Theme::SunsetPlum);
    window.draw(rule);

    sf::RectangleShape accent(sf::Vector2f(180.0f * Animation::EaseOutCubic((elapsed - 0.08f) / 0.4f), 2.0f));
    accent.setPosition(left, panel.top + 101.0f);
    accent.setFillColor(Theme::SunsetAmber);
    window.draw(accent);

    // ---- Size fields ----
    bool caretOn = std::fmod(Theme::s_time, 1.0f) < 0.55f;

    auto drawField = [&](const sf::FloatRect& b, const std::string& label, const std::string& val, bool active) {
        float t = Theme::AnimateHover(b, b.contains(mPos));
        float centerY = b.top + b.height * 0.5f;

        if (active) {
            sf::RectangleShape glow(sf::Vector2f(b.width + 6.0f, b.height + 6.0f));
            glow.setPosition(b.left - 3.0f, b.top - 3.0f);
            glow.setFillColor(Theme::WithAlpha(Theme::SunsetAmber, 40.0f));
            window.draw(glow);
        }

        sf::RectangleShape box(sf::Vector2f(b.width, b.height));
        box.setPosition(b.left, b.top);
        box.setFillColor(Theme::WithAlpha(Theme::PanelInset, 240.0f));
        box.setOutlineThickness(active ? 1.5f : 1.0f);
        box.setOutlineColor(active ? Theme::SunsetGold : Theme::Mix(Theme::SunsetPlum, Theme::SunsetPeach, t));
        window.draw(box);

        Theme::DrawCrispText(window, font, label, 15, b.left, b.top - 14.0f, active ? Theme::SunsetGold : Theme::TextSecondary, sf::Color::Transparent, false, true);
        Theme::DrawCrispText(window, font, val, 20, b.left + 16.0f, centerY, Theme::TextPrimary, sf::Color::Transparent, false, true);

        if (active && caretOn) {
            sf::RectangleShape caret(sf::Vector2f(2.0f, 22.0f));
            caret.setPosition(std::floor(b.left + 16.0f + (val.empty() ? 0.0f : Theme::MeasureText(font, val, 20) + 3.0f)), std::floor(centerY - 11.0f));
            caret.setFillColor(Theme::SunsetGold);
            window.draw(caret);
        }

        // Out-of-range sizes are refused on apply, so say so while typing.
        int value = parseSize(val);
        std::string suffix = value < kMinSize ? ("MIN " + std::to_string(kMinSize)) : value > kMaxSize ? ("MAX " + std::to_string(kMaxSize)) : "PX";
        float suffixW = Theme::MeasureText(font, suffix, 15);
        Theme::DrawCrispText(window, font, suffix, 15, b.left + b.width - 14.0f - suffixW, centerY, isValidSize(value) ? Theme::TextMuted : Theme::SunsetCoral, sf::Color::Transparent, false, true);
        };

    sf::FloatRect wBox = widthBox();
    sf::FloatRect hBox = heightBox();
    drawField(wBox, "WIDTH", m_resizeWBuf, m_activeResizeField == 0);
    drawField(hBox, "HEIGHT", m_resizeHBuf, m_activeResizeField == 1);
    Theme::DrawCrispText(window, font, "x", 16, (wBox.left + wBox.width + hBox.left) * 0.5f, wBox.top + wBox.height * 0.5f, Theme::TextMuted, sf::Color::Transparent, true, true);

    // ---- Before / after ----
    sf::FloatRect card = compareCard();
    sf::RectangleShape cardBg(sf::Vector2f(card.width, card.height));
    cardBg.setPosition(card.left, card.top);
    cardBg.setFillColor(Theme::WithAlpha(Theme::PanelInset, 240.0f));
    cardBg.setOutlineThickness(1.0f);
    cardBg.setOutlineColor(Theme::SunsetPlum);
    window.draw(cardBg);

    // Both sizes drawn to the same scale, so the change in shape is visible at a glance.
    sf::FloatRect stage(card.left + 16.0f, card.top + 14.0f, 150.0f, card.height - 28.0f);
    float largestW = static_cast<float>(std::max(current.x, valid ? static_cast<unsigned int>(newW) : 0u));
    float largestH = static_cast<float>(std::max(current.y, valid ? static_cast<unsigned int>(newH) : 0u));
    float stageScale = std::min(stage.width / std::max(1.0f, largestW), stage.height / std::max(1.0f, largestH));

    auto drawOutline = [&](float w, float h, sf::Color fill, sf::Color edge, float thickness) {
        float visW = std::floor(std::max(4.0f, w * stageScale));
        float visH = std::floor(std::max(4.0f, h * stageScale));
        sf::RectangleShape shape(sf::Vector2f(visW, visH));
        shape.setPosition(std::floor(stage.left + (stage.width - visW) * 0.5f), std::floor(stage.top + (stage.height - visH) * 0.5f));
        shape.setFillColor(fill);
        shape.setOutlineThickness(thickness);
        shape.setOutlineColor(edge);
        window.draw(shape);
        };

    drawOutline(static_cast<float>(current.x), static_cast<float>(current.y), Theme::WithAlpha(Theme::SunsetPlum, 110.0f), Theme::SunsetViolet, 1.0f);
    if (valid) drawOutline(static_cast<float>(newW), static_cast<float>(newH), Theme::WithAlpha(Theme::SunsetAmber, 36.0f), Theme::SunsetGold, 1.5f);

    int infoRow = 0;
    auto drawInfo = [&](const std::string& label, const std::string& value, sf::Color valueColor) {
        float y = card.top + 26.0f + static_cast<float>(infoRow++) * 28.0f;
        Theme::DrawCrispText(window, font, label, 15, card.left + 194.0f, y, Theme::TextMuted, sf::Color::Transparent, false, true);
        Theme::DrawCrispText(window, font, value, 16, card.left + card.width - 20.0f - Theme::MeasureText(font, value, 16), y, valueColor, sf::Color::Transparent, false, true);
        };

    std::string rangeNote = std::to_string(kMinSize) + " TO " + std::to_string(kMaxSize) + " PX";
    drawInfo("CURRENT", std::to_string(current.x) + " x " + std::to_string(current.y), Theme::SunsetViolet);
    drawInfo("NEW", valid ? (std::to_string(newW) + " x " + std::to_string(newH)) : rangeNote, valid ? Theme::SunsetGold : Theme::SunsetCoral);
    drawInfo("ASPECT", valid ? aspectLabel(newW, newH) : "-", Theme::TextPrimary);

    // ---- Buttons ----
    // Apply is the main action here, so it gets the filled amber treatment; it dims while the size is invalid.
    {
        sf::FloatRect bounds = applyButton();
        bool hovered = bounds.contains(mPos);
        float t = Theme::AnimateHover(bounds, hovered) * (valid ? 1.0f : 0.0f);
        sf::FloatRect b = shifted(bounds, 0.0f, (valid && hovered && mouseDown) ? 1.0f : 0.0f);

        if (valid) {
            sf::ConvexShape glow = Theme::ChamferedRect(b.left - 4.0f, b.top - 4.0f, b.width + 8.0f, b.height + 8.0f, 7.0f);
            glow.setFillColor(Theme::WithAlpha(Theme::SunsetGold, 26.0f + 12.0f * std::sin(Theme::s_time * 3.0f) + 46.0f * t));
            window.draw(glow);
        }

        sf::Color face = Theme::Mix(Theme::SunsetAmber, Theme::SunsetGold, t);
        sf::ConvexShape body = Theme::ChamferedRect(b.left, b.top, b.width, b.height, 4.0f);
        body.setFillColor(valid ? face : Theme::Mix(face, Theme::SunsetDeepDark, 0.62f));
        body.setOutlineThickness(1.0f);
        body.setOutlineColor(valid ? Theme::SunsetGlow : Theme::SunsetPlum);
        window.draw(body);

        if (valid) {
            sf::RectangleShape lower(sf::Vector2f(b.width, b.height * 0.5f - 4.0f));
            lower.setPosition(b.left, b.top + b.height * 0.5f);
            lower.setFillColor(sf::Color(120, 50, 10, 34));
            window.draw(lower);
        }

        Theme::DrawCrispText(window, font, "APPLY", 20, b.left + b.width * 0.5f, b.top + b.height * 0.5f, valid ? Theme::SunsetDeepDark : Theme::TextMuted, sf::Color::Transparent, true, true);
    }

    sf::FloatRect cancel = cancelButton();
    Theme::DrawSunsetButton(window, cancel, "CANCEL", font, 18, false, cancel.contains(mPos), false, 1.0f);

    Theme::DrawCrispText(window, font, "TAB  SWITCH FIELD   |   ENTER  APPLY   |   ESC  CANCEL", 15, panel.left + panel.width * 0.5f, panel.top + 434.0f, Theme::TextMuted, sf::Color::Transparent, true, true);

    window.setView(savedView);
}

bool UIManager::handleResizeModalEvent(const sf::Event& event, sf::RenderWindow& window, Canvas& canvas, Timeline& timeline) {
    if (!m_showResizeModal) return false;

    sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

    auto applyResize = [&]() {
        if (m_resizeWBuf.empty() || m_resizeHBuf.empty()) {
            showMessage("Invalid dimensions entered", sf::Color::Red);
            return;
        }
        int nw = parseSize(m_resizeWBuf);
        int nh = parseSize(m_resizeHBuf);
        if (isValidSize(nw) && isValidSize(nh)) {
            canvas.resizeCanvas(nw, nh);
            if (projManager && !activeProjectPath.empty()) {
                projManager->saveProjectAs(activeProjectPath, activeProjectName, canvas, static_cast<int>(timeline.getFps()), canvas.getPixelMode());
            }
            showMessage("Canvas Resized to " + std::to_string(nw) + "x" + std::to_string(nh), sf::Color::Green);
            m_showResizeModal = false;
        }
        else {
            showMessage("Size must be between " + std::to_string(kMinSize) + " and " + std::to_string(kMaxSize) + " px", sf::Color::Red);
        }
        };

    if (event.type == sf::Event::TextEntered) {
        if (event.text.unicode == '\b') {
            if (m_activeResizeField == 0 && !m_resizeWBuf.empty()) m_resizeWBuf.pop_back();
            else if (m_activeResizeField == 1 && !m_resizeHBuf.empty()) m_resizeHBuf.pop_back();
        }
        else if (event.text.unicode >= '0' && event.text.unicode <= '9') {
            if (m_activeResizeField == 0 && m_resizeWBuf.length() < kMaxDigits) m_resizeWBuf += static_cast<char>(event.text.unicode);
            else if (m_activeResizeField == 1 && m_resizeHBuf.length() < kMaxDigits) m_resizeHBuf += static_cast<char>(event.text.unicode);
        }
        return true;
    }

    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Escape) {
            m_showResizeModal = false;
            return true;
        }
        if (event.key.code == sf::Keyboard::Tab) {
            m_activeResizeField = (m_activeResizeField == 0) ? 1 : 0;
            return true;
        }
        if (event.key.code == sf::Keyboard::Enter) {
            applyResize();
            return true;
        }
    }

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        if (widthBox().contains(mousePos)) m_activeResizeField = 0;
        else if (heightBox().contains(mousePos)) m_activeResizeField = 1;
        else if (applyButton().contains(mousePos)) applyResize();
        else if (cancelButton().contains(mousePos)) m_showResizeModal = false;
        return true;
    }

    return true;
}
