#include "PerspectivePanel.h"
#include "../UI/UITheme.h"
#include <algorithm>
#include <cmath>

PerspectivePanel::PerspectivePanel() : m_pm(nullptr), m_position(64.f, 78.f), m_size(280.f, 510.f), m_presetsOpen(false) {}

void PerspectivePanel::init(PerspectiveManager* pm) {
    m_pm = pm;
    m_font.loadFromFile("assets/font.otf");
    m_position = sf::Vector2f(64.f, 78.f);
}

void PerspectivePanel::update(float dt) {}

void PerspectivePanel::drawButton(sf::RenderWindow& window, sf::FloatRect bounds, const std::string& text, sf::Color bgColor) {
    bool hovered = bounds.contains(window.mapPixelToCoords(sf::Mouse::getPosition(window)));
    WisdomUI::Theme::DrawSunsetButton(window, bounds, text, m_font, 12, false, hovered, false, 1.0f);
}

void PerspectivePanel::drawToggle(sf::RenderWindow& window, sf::FloatRect bounds, const std::string& text, bool state) {
    bool hovered = bounds.contains(window.mapPixelToCoords(sf::Mouse::getPosition(window)));
    WisdomUI::Theme::DrawSunsetButton(window, bounds, text, m_font, 12, state, hovered, state, 1.0f);
}

void PerspectivePanel::draw(sf::RenderWindow& window) {
    sf::FloatRect panelBounds(m_position.x, m_position.y, m_size.x, m_size.y);
    WisdomUI::Theme::DrawSunsetPanel(window, panelBounds, 1.0f);

    sf::FloatRect headerGrip(m_position.x + 8.f, m_position.y + 6.f, m_size.x - 16.f, 28.f);
    sf::RectangleShape gripBg(sf::Vector2f(headerGrip.width, headerGrip.height));
    gripBg.setPosition(headerGrip.left, headerGrip.top);
    gripBg.setFillColor(WisdomUI::Theme::SunsetDeepDark);
    gripBg.setOutlineThickness(1.f);
    gripBg.setOutlineColor(WisdomUI::Theme::SunsetPlum);
    window.draw(gripBg);

    WisdomUI::Theme::DrawCrispText(window, m_font, ":: PERSPECTIVE GRID ::", 13, headerGrip.left + headerGrip.width / 2.0f, headerGrip.top + headerGrip.height / 2.0f, WisdomUI::Theme::SunsetAmber, sf::Color(14, 6, 20), true, true);

    PerspectiveConfig* activeCfg = m_pm ? m_pm->getActiveConfig() : nullptr;
    float y = m_position.y + 42.f;

    drawButton(window, sf::FloatRect(m_position.x + 12.f, y, 122.f, 28.f), "+ 1 Point", WisdomUI::Theme::SunsetSkyMid);
    drawButton(window, sf::FloatRect(m_position.x + 144.f, y, 122.f, 28.f), "+ 2 Point", WisdomUI::Theme::SunsetSkyMid);
    y += 36.f;
    drawButton(window, sf::FloatRect(m_position.x + 12.f, y, 122.f, 28.f), "+ 3 Point", WisdomUI::Theme::SunsetSkyMid);
    drawButton(window, sf::FloatRect(m_position.x + 144.f, y, 122.f, 28.f), "Custom", WisdomUI::Theme::SunsetSkyMid);
    y += 38.f;

    drawButton(window, sf::FloatRect(m_position.x + 12.f, y, 254.f, 28.f), "Presets Library (Click)", WisdomUI::Theme::SunsetCoral);
    y += 38.f;

    if (m_presetsOpen && m_pm) {
        auto names = m_pm->getPresetNames();
        for (const auto& name : names) {
            drawButton(window, sf::FloatRect(m_position.x + 12.f, y, 254.f, 24.f), name, WisdomUI::Theme::SunsetSkyTop);
            y += 28.f;
        }
        y += 6.f;
    }

    if (activeCfg) {
        WisdomUI::Theme::DrawCrispText(window, m_font, "Active: " + activeCfg->name, 13, m_position.x + 16.f, y + 2.f, WisdomUI::Theme::SunsetGold);
        y += 26.f;

        drawToggle(window, sf::FloatRect(m_position.x + 12.f, y, 122.f, 28.f), "Visible", activeCfg->guideSettings.visible);
        drawToggle(window, sf::FloatRect(m_position.x + 144.f, y, 122.f, 28.f), "Locked", activeCfg->guideSettings.locked);
        y += 36.f;
        drawToggle(window, sf::FloatRect(m_position.x + 12.f, y, 254.f, 28.f), "Brush Snap", activeCfg->guideSettings.brushSnap);
        y += 36.f;

        WisdomUI::Theme::DrawCrispText(window, m_font, "Density: " + std::to_string(activeCfg->guideSettings.density), 13, m_position.x + 16.f, y + 2.f, WisdomUI::Theme::TextSecondary);
    }
}

void PerspectivePanel::handleEvent(const sf::Event& event, sf::Vector2f mousePos, sf::Vector2u canvasSize) {
    sf::FloatRect headerGrip(m_position.x, m_position.y, m_size.x, 34.f);

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        if (headerGrip.contains(mousePos)) {
            m_isDraggingPanel = true;
            m_dragOffset = mousePos - m_position;
            return;
        }

        if (!sf::FloatRect(m_position.x, m_position.y, m_size.x, m_size.y).contains(mousePos)) return;

        float bx = m_position.x;
        float y = m_position.y + 42.f;

        if (sf::FloatRect(bx + 12.f, y, 122.f, 28.f).contains(mousePos) && m_pm) m_pm->addConfig(PerspectiveMode::OnePoint, canvasSize);
        if (sf::FloatRect(bx + 144.f, y, 122.f, 28.f).contains(mousePos) && m_pm) m_pm->addConfig(PerspectiveMode::TwoPoint, canvasSize);
        y += 36.f;
        if (sf::FloatRect(bx + 12.f, y, 122.f, 28.f).contains(mousePos) && m_pm) m_pm->addConfig(PerspectiveMode::ThreePoint, canvasSize);
        if (sf::FloatRect(bx + 144.f, y, 122.f, 28.f).contains(mousePos) && m_pm) m_pm->addConfig(PerspectiveMode::Custom, canvasSize);
        y += 38.f;

        if (sf::FloatRect(bx + 12.f, y, 254.f, 28.f).contains(mousePos)) m_presetsOpen = !m_presetsOpen;
        y += 38.f;

        if (m_presetsOpen && m_pm) {
            auto names = m_pm->getPresetNames();
            for (const auto& name : names) {
                if (sf::FloatRect(bx + 12.f, y, 254.f, 24.f).contains(mousePos)) {
                    m_pm->loadPreset(name, canvasSize);
                    m_presetsOpen = false;
                    return;
                }
                y += 28.f;
            }
            y += 6.f;
        }

        PerspectiveConfig* activeCfg = m_pm ? m_pm->getActiveConfig() : nullptr;
        if (activeCfg) {
            y += 26.f;
            if (sf::FloatRect(bx + 12.f, y, 122.f, 28.f).contains(mousePos)) activeCfg->guideSettings.visible = !activeCfg->guideSettings.visible;
            if (sf::FloatRect(bx + 144.f, y, 122.f, 28.f).contains(mousePos)) activeCfg->guideSettings.locked = !activeCfg->guideSettings.locked;
            y += 36.f;
            if (sf::FloatRect(bx + 12.f, y, 254.f, 28.f).contains(mousePos)) activeCfg->guideSettings.brushSnap = !activeCfg->guideSettings.brushSnap;
        }
    }
    else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        m_isDraggingPanel = false;
    }
    else if (event.type == sf::Event::MouseMoved && m_isDraggingPanel) {
        m_position = mousePos - m_dragOffset;
        m_position.x = std::clamp(m_position.x, 56.f, 1920.f - m_size.x);
        m_position.y = std::clamp(m_position.y, 40.f, 1080.f - m_size.y);
    }
}