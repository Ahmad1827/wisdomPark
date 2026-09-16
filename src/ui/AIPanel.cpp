#include "AIPanel.h"
#include "../core/ColorManager.h"
#include "../UI/UITheme.h"
#include <algorithm>

AIPanel::AIPanel()
    : position(64.f, 78.f),
    size(280.f, 190.f),
    isVisible(false),
    isDraggingPanel(false) {}

void AIPanel::init() {
    font.loadFromFile("assets/font.otf");
    position = sf::Vector2f(64.f, 78.f);
}

void AIPanel::toggle() {
    isVisible = !isVisible;
}

bool AIPanel::getIsVisible() const {
    return isVisible;
}

void AIPanel::update(float dt) {
    if (!isVisible) return;

    float bx = position.x;
    float by = position.y;

    headerGripBounds = sf::FloatRect(bx + 8.f, by + 6.f, size.x - 16.f, 26.f);
    suggestBtnBounds = sf::FloatRect(bx + 16.f, by + 80.f, size.x - 32.f, 42.f);
    closeBtnBounds = sf::FloatRect(bx + 16.f, by + 138.f, size.x - 32.f, 28.f);
}

void AIPanel::draw(sf::RenderWindow& window) {
    if (!isVisible) return;

    sf::FloatRect panelBounds(position.x, position.y, size.x, size.y);
    WisdomUI::Theme::DrawSunsetPanel(window, panelBounds, 1.0f);

    sf::RectangleShape gripBg(sf::Vector2f(headerGripBounds.width, headerGripBounds.height));
    gripBg.setPosition(headerGripBounds.left, headerGripBounds.top);
    gripBg.setFillColor(WisdomUI::Theme::SunsetDeepDark);
    gripBg.setOutlineThickness(1.f);
    gripBg.setOutlineColor(WisdomUI::Theme::SunsetPlum);
    window.draw(gripBg);

    WisdomUI::Theme::DrawCrispText(window, font, ":: COLOR ASSISTANT ::", 12, headerGripBounds.left + headerGripBounds.width / 2.0f, headerGripBounds.top + headerGripBounds.height / 2.0f, WisdomUI::Theme::SunsetAmber, sf::Color(14, 6, 20), true, true);

    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

    WisdomUI::Theme::DrawCrispText(window, font, "Generate shading advice ramp", 10, position.x + 16.f, position.y + 42.f, WisdomUI::Theme::TextSecondary);
    WisdomUI::Theme::DrawCrispText(window, font, "from active brush color.", 10, position.x + 16.f, position.y + 56.f, WisdomUI::Theme::TextSecondary);

    bool hovSuggest = suggestBtnBounds.contains(mPos);
    WisdomUI::Theme::DrawSunsetButton(window, suggestBtnBounds, "+ Get Palette Advice", font, 12, false, hovSuggest, true, 1.0f);

    bool hovClose = closeBtnBounds.contains(mPos);
    WisdomUI::Theme::DrawSunsetButton(window, closeBtnBounds, "Close", font, 11, false, hovClose, false, 1.0f);
}

bool AIPanel::handleEvent(const sf::Event& event, sf::Vector2f mousePos) {
    if (!isVisible) return false;

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        if (headerGripBounds.contains(mousePos)) {
            isDraggingPanel = true;
            dragOffset = mousePos - position;
            return true;
        }
        if (sf::FloatRect(position.x, position.y, size.x, size.y).contains(mousePos)) {
            return true;
        }
    }
    else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        if (isDraggingPanel) {
            isDraggingPanel = false;
            return true;
        }
    }
    else if (event.type == sf::Event::MouseMoved && isDraggingPanel) {
        position = mousePos - dragOffset;
        position.x = std::clamp(position.x, 56.f, 1920.f - size.x);
        position.y = std::clamp(position.y, 40.f, 1080.f - size.y);
        return true;
    }

    return sf::FloatRect(position.x, position.y, size.x, size.y).contains(mousePos);
}

std::string AIPanel::handleClick(sf::Vector2f mousePos) {
    if (!isVisible) return "";

    if (suggestBtnBounds.contains(mousePos)) return "action:get_advice";
    if (closeBtnBounds.contains(mousePos)) {
        toggle();
        return "close";
    }
    return "";
}

std::vector<sf::Color> AIPanel::createColorAdvice(sf::Color base, int variation) {
    int vMode = std::abs(variation) % 6;
    float h = 0.f, s = 0.f, v = 0.f;
    ColorManager::rgbToHsv(base, h, s, v);

    if (v < 0.12f) {
        if (vMode == 0) return { sf::Color(115, 130, 165), sf::Color(65, 75, 105), sf::Color(32, 34, 52), sf::Color(12, 12, 18) };
        if (vMode == 1) return { sf::Color(165, 125, 95), sf::Color(105, 75, 60), sf::Color(55, 35, 30), sf::Color(22, 14, 12) };
        if (vMode == 2) return { sf::Color(100, 185, 175), sf::Color(45, 110, 115), sf::Color(22, 55, 65), sf::Color(10, 20, 26) };
        if (vMode == 3) return { sf::Color(175, 105, 155), sf::Color(115, 55, 95), sf::Color(60, 25, 50), sf::Color(25, 10, 20) };
        if (vMode == 4) return { sf::Color(180, 180, 195), sf::Color(120, 120, 135), sf::Color(60, 60, 75), sf::Color(18, 18, 24) };
        return { sf::Color(150, 135, 110), sf::Color(90, 80, 65), sf::Color(45, 40, 32), sf::Color(16, 14, 10) };
    }

    if (v > 0.92f && s < 0.10f) {
        if (vMode == 0) return { sf::Color(255, 255, 255), sf::Color(245, 235, 210), sf::Color(190, 175, 160), sf::Color(125, 110, 105) };
        if (vMode == 1) return { sf::Color(255, 255, 255), sf::Color(230, 245, 250), sf::Color(165, 190, 210), sf::Color(105, 125, 155) };
        if (vMode == 2) return { sf::Color(255, 255, 255), sf::Color(245, 230, 245), sf::Color(195, 165, 200), sf::Color(135, 105, 145) };
        if (vMode == 3) return { sf::Color(255, 255, 255), sf::Color(235, 245, 230), sf::Color(175, 195, 170), sf::Color(115, 135, 110) };
        if (vMode == 4) return { sf::Color(255, 255, 255), sf::Color(225, 225, 235), sf::Color(160, 160, 175), sf::Color(95, 95, 115) };
        return { sf::Color(255, 255, 255), sf::Color(250, 240, 220), sf::Color(180, 170, 150), sf::Color(110, 100, 85) };
    }

    float hL = h, sL = s, vL = v;
    float hS = h, sS = s, vS = v;
    float hD = h, sD = s, vD = v;

    if (vMode == 0) {
        hL = (h >= 50.f && h <= 230.f) ? std::max(45.f, h - 22.f) : std::min(55.f, h + 16.f);
        sL = std::max(0.12f, s * 0.72f);
        vL = std::min(1.0f, v * 1.25f + 0.12f);
        hS = (h < 245.f && h >= 45.f) ? std::min(255.f, h + 24.f) : std::max(0.f, h - 20.f);
        sS = std::min(1.0f, s * 1.22f + 0.08f);
        vS = std::max(0.08f, v * 0.68f);
        hD = (hS < 255.f) ? std::min(270.f, hS + 16.f) : hS;
        sD = std::min(1.0f, sS * 1.15f + 0.06f);
        vD = std::max(0.04f, vS * 0.58f);
    }
    else if (vMode == 1) {
        hL = std::min(210.f, std::max(170.f, h + 18.f));
        sL = std::max(0.10f, s * 0.68f);
        vL = std::min(1.0f, v * 1.22f + 0.10f);
        hS = (h > 200.f) ? std::max(200.f, h - 25.f) : std::min(240.f, h + 30.f);
        sS = std::min(1.0f, s * 1.15f + 0.10f);
        vS = std::max(0.08f, v * 0.62f);
        hD = std::min(265.f, hS + 18.f);
        sD = std::min(1.0f, sS * 1.20f + 0.08f);
        vD = std::max(0.04f, vS * 0.52f);
    }
    else if (vMode == 2) {
        hL = h;
        sL = std::max(0.05f, s * 0.50f);
        vL = 1.0f;
        hS = (h < 240.f) ? std::min(250.f, h + 15.f) : h;
        sS = std::min(1.0f, s * 1.35f + 0.12f);
        vS = std::max(0.06f, v * 0.55f);
        hD = hS;
        sD = std::min(1.0f, sS * 1.20f + 0.10f);
        vD = std::max(0.03f, vS * 0.50f);
    }
    else if (vMode == 3) {
        hL = (h >= 60.f) ? h - 12.f : h + 12.f;
        sL = std::max(0.08f, s * 0.55f);
        vL = std::min(1.0f, v * 1.15f + 0.15f);
        hS = (h <= 260.f) ? h + 12.f : h - 12.f;
        sS = std::max(0.15f, s * 0.85f);
        vS = std::max(0.15f, v * 0.78f);
        hD = hS;
        sD = std::min(1.0f, sS * 1.10f);
        vD = std::max(0.08f, vS * 0.72f);
    }
    else if (vMode == 4) {
        hL = std::fmod(h + 30.f, 360.f);
        sL = std::max(0.15f, s * 0.80f);
        vL = std::min(1.0f, v * 1.20f + 0.10f);
        hS = std::fmod(h + 180.f, 360.f);
        sS = std::min(1.0f, s * 1.10f + 0.15f);
        vS = std::max(0.10f, v * 0.65f);
        hD = hS;
        sD = std::min(1.0f, sS * 1.20f);
        vD = std::max(0.05f, vS * 0.55f);
    }
    else {
        hL = std::fmod(h + 340.f, 360.f);
        sL = std::min(1.0f, s * 1.30f);
        vL = std::min(1.0f, v * 1.30f);
        hS = std::fmod(h + 40.f, 360.f);
        sS = std::max(0.20f, s * 0.70f);
        vS = std::max(0.10f, v * 0.50f);
        hD = std::fmod(h + 60.f, 360.f);
        sD = std::max(0.30f, s * 0.60f);
        vD = std::max(0.04f, v * 0.35f);
    }

    sf::Color highlight = ColorManager::hsvToRgb(hL, sL, vL);
    sf::Color shadow = ColorManager::hsvToRgb(hS, sS, vS);
    sf::Color deepShadow = ColorManager::hsvToRgb(hD, sD, vD);

    return { highlight, base, shadow, deepShadow };
}