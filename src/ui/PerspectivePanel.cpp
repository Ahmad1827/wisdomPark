#include "PerspectivePanel.h"

PerspectivePanel::PerspectivePanel() : m_pm(nullptr), m_presetsOpen(false) {}

void PerspectivePanel::init(PerspectiveManager* pm) {
    m_pm = pm;
    m_font.loadFromFile("assets/font.otf");
    m_background.setSize(sf::Vector2f(260.f, 500.f));
    m_background.setFillColor(sf::Color(25, 25, 30, 240));
    m_background.setOutlineThickness(1.f);
    m_background.setOutlineColor(sf::Color(100, 100, 110));
    m_background.setPosition(15.f, 300.f);
}

void PerspectivePanel::update(float dt) {}

void PerspectivePanel::drawButton(sf::RenderWindow& window, sf::FloatRect bounds, const std::string& text, sf::Color bgColor) {
    sf::RectangleShape btn(sf::Vector2f(bounds.width, bounds.height));
    btn.setPosition(bounds.left, bounds.top);
    btn.setFillColor(bgColor);
    btn.setOutlineThickness(1.f);
    btn.setOutlineColor(sf::Color(80, 80, 90));
    window.draw(btn);
    sf::Text t(text, m_font, 12);
    t.setPosition(bounds.left + 10.f, bounds.top + 6.f);
    t.setFillColor(sf::Color::White);
    window.draw(t);
}

void PerspectivePanel::drawToggle(sf::RenderWindow& window, sf::FloatRect bounds, const std::string& text, bool state) {
    drawButton(window, bounds, text, state ? sf::Color(0, 122, 204) : sf::Color(40, 40, 45));
}

void PerspectivePanel::draw(sf::RenderWindow& window) {
    window.draw(m_background);
    sf::Text title("PERSPECTIVE SYSTEM", m_font, 14);
    title.setPosition(m_background.getPosition().x + 10.f, m_background.getPosition().y + 10.f);
    title.setFillColor(sf::Color(255, 200, 100));
    window.draw(title);

    PerspectiveConfig* activeCfg = m_pm->getActiveConfig();
    float y = m_background.getPosition().y + 40.f;

    drawButton(window, sf::FloatRect(m_background.getPosition().x + 10.f, y, 115.f, 25.f), "+ 1 Point", sf::Color(50, 50, 60));
    drawButton(window, sf::FloatRect(m_background.getPosition().x + 135.f, y, 115.f, 25.f), "+ 2 Point", sf::Color(50, 50, 60));
    y += 35.f;
    drawButton(window, sf::FloatRect(m_background.getPosition().x + 10.f, y, 115.f, 25.f), "+ 3 Point", sf::Color(50, 50, 60));
    drawButton(window, sf::FloatRect(m_background.getPosition().x + 135.f, y, 115.f, 25.f), "Custom", sf::Color(50, 50, 60));
    y += 40.f;

    drawButton(window, sf::FloatRect(m_background.getPosition().x + 10.f, y, 240.f, 25.f), "Presets Library (Click)", sf::Color(0, 150, 136));
    y += 40.f;

    if (m_presetsOpen) {
        auto names = m_pm->getPresetNames();
        for (const auto& name : names) {
            drawButton(window, sf::FloatRect(m_background.getPosition().x + 10.f, y, 240.f, 20.f), name, sf::Color(30, 30, 35));
            y += 22.f;
        }
        y += 10.f;
    }

    if (activeCfg) {
        sf::Text s("Active: " + activeCfg->name, m_font, 12);
        s.setPosition(m_background.getPosition().x + 10.f, y);
        s.setFillColor(sf::Color::White);
        window.draw(s);
        y += 20.f;

        drawToggle(window, sf::FloatRect(m_background.getPosition().x + 10.f, y, 115.f, 25.f), "Visible", activeCfg->guideSettings.visible);
        drawToggle(window, sf::FloatRect(m_background.getPosition().x + 135.f, y, 115.f, 25.f), "Locked", activeCfg->guideSettings.locked);
        y += 35.f;
        drawToggle(window, sf::FloatRect(m_background.getPosition().x + 10.f, y, 240.f, 25.f), "Brush Snap", activeCfg->guideSettings.brushSnap);
        y += 35.f;

        sf::Text den("Density: " + std::to_string(activeCfg->guideSettings.density), m_font, 12);
        den.setPosition(m_background.getPosition().x + 10.f, y);
        den.setFillColor(sf::Color::White);
        window.draw(den);
    }
}

void PerspectivePanel::handleEvent(const sf::Event& event, sf::Vector2f mousePos, sf::Vector2u canvasSize) {
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        if (!m_background.getGlobalBounds().contains(mousePos)) return;

        float bx = m_background.getPosition().x;
        float y = m_background.getPosition().y + 40.f;

        if (sf::FloatRect(bx + 10.f, y, 115.f, 25.f).contains(mousePos)) m_pm->addConfig(PerspectiveMode::OnePoint, canvasSize);
        if (sf::FloatRect(bx + 135.f, y, 115.f, 25.f).contains(mousePos)) m_pm->addConfig(PerspectiveMode::TwoPoint, canvasSize);
        y += 35.f;
        if (sf::FloatRect(bx + 10.f, y, 115.f, 25.f).contains(mousePos)) m_pm->addConfig(PerspectiveMode::ThreePoint, canvasSize);
        if (sf::FloatRect(bx + 135.f, y, 115.f, 25.f).contains(mousePos)) m_pm->addConfig(PerspectiveMode::Custom, canvasSize);
        y += 40.f;

        if (sf::FloatRect(bx + 10.f, y, 240.f, 25.f).contains(mousePos)) m_presetsOpen = !m_presetsOpen;
        y += 40.f;

        if (m_presetsOpen) {
            auto names = m_pm->getPresetNames();
            for (const auto& name : names) {
                if (sf::FloatRect(bx + 10.f, y, 240.f, 20.f).contains(mousePos)) {
                    m_pm->loadPreset(name, canvasSize);
                    m_presetsOpen = false;
                    return;
                }
                y += 22.f;
            }
            y += 10.f;
        }

        PerspectiveConfig* activeCfg = m_pm->getActiveConfig();
        if (activeCfg) {
            y += 20.f;
            if (sf::FloatRect(bx + 10.f, y, 115.f, 25.f).contains(mousePos)) activeCfg->guideSettings.visible = !activeCfg->guideSettings.visible;
            if (sf::FloatRect(bx + 135.f, y, 115.f, 25.f).contains(mousePos)) activeCfg->guideSettings.locked = !activeCfg->guideSettings.locked;
            y += 35.f;
            if (sf::FloatRect(bx + 10.f, y, 240.f, 25.f).contains(mousePos)) activeCfg->guideSettings.brushSnap = !activeCfg->guideSettings.brushSnap;
        }
    }
}