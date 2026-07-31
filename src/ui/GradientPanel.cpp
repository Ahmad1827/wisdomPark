#include "GradientPanel.h"
#include <algorithm>
#include <string>

GradientPanel::GradientPanel() : m_config(nullptr), m_draggedStopIndex(-1), m_selectedStopIndex(-1) {}

void GradientPanel::init(GradientConfig* config) {
    m_config = config;
    m_font.loadFromFile("assets/font.otf");
    m_background.setSize(sf::Vector2f(280.f, 480.f));
    m_background.setFillColor(sf::Color(25, 25, 30, 240));
    m_background.setOutlineThickness(1.f);
    m_background.setOutlineColor(sf::Color(100, 100, 110));
    m_background.setPosition(15.f, 250.f);
}

void GradientPanel::update(float dt) {}

void GradientPanel::drawButton(sf::RenderWindow& window, sf::FloatRect bounds, const std::string& text, sf::Color bgColor) {
    sf::RectangleShape btn(sf::Vector2f(bounds.width, bounds.height));
    btn.setPosition(bounds.left, bounds.top);
    btn.setFillColor(bgColor);
    btn.setOutlineThickness(1.f);
    btn.setOutlineColor(sf::Color(80, 80, 90));
    window.draw(btn);
    sf::Text t(text, m_font, 12);
    t.setPosition(bounds.left + 10.f, bounds.top + 4.f);
    t.setFillColor(sf::Color::White);
    window.draw(t);
}

void GradientPanel::drawToggle(sf::RenderWindow& window, sf::FloatRect bounds, const std::string& text, bool state) {
    drawButton(window, bounds, text, state ? sf::Color(0, 122, 204) : sf::Color(40, 40, 45));
}

void GradientPanel::sortStops() {
    if (m_config) {
        std::sort(m_config->stops.begin(), m_config->stops.end());
    }
}

void GradientPanel::setSelectedColor(sf::Color color) {
    if (m_config && m_selectedStopIndex >= 0 && m_selectedStopIndex < static_cast<int>(m_config->stops.size())) {
        m_config->stops[m_selectedStopIndex].color = color;
    }
}

void GradientPanel::updateBlendMode() {
    if (!m_config) return;
    switch (m_config->blendModeIndex) {
    case 0: m_config->blendMode = sf::BlendNone; break;
    case 1: m_config->blendMode = sf::BlendAlpha; break;
    case 2: m_config->blendMode = sf::BlendMultiply; break;
    case 3: m_config->blendMode = sf::BlendAdd; break;
    case 4: m_config->blendMode = sf::BlendMode(sf::BlendMode::One, sf::BlendMode::OneMinusSrcColor, sf::BlendMode::Add, sf::BlendMode::One, sf::BlendMode::OneMinusSrcAlpha, sf::BlendMode::Add); break;
    }
}

void GradientPanel::draw(sf::RenderWindow& window) {
    window.draw(m_background);
    sf::Text title("GRADIENT", m_font, 14);
    title.setPosition(m_background.getPosition().x + 10.f, m_background.getPosition().y + 10.f);
    title.setFillColor(sf::Color(255, 200, 100));
    window.draw(title);

    if (!m_config) return;

    float bx = m_background.getPosition().x;
    float y = m_background.getPosition().y + 35.f;

    drawButton(window, sf::FloatRect(bx + 10.f, y, 260.f, 20.f),
        m_config->type == GradientType::Linear ? "Type: Linear" :
        m_config->type == GradientType::Radial ? "Type: Radial" :
        m_config->type == GradientType::Diamond ? "Type: Diamond" :
        m_config->type == GradientType::Angle ? "Type: Angle" : "Type: Reflected",
        sf::Color(50, 50, 60));
    y += 25.f;

    drawButton(window, sf::FloatRect(bx + 10.f, y, 260.f, 20.f),
        m_config->interpolation == GradientInterpolation::RGB ? "Interp: RGB" :
        m_config->interpolation == GradientInterpolation::HSV ? "Interp: HSV" : "Interp: Constant",
        sf::Color(50, 50, 60));
    y += 25.f;

    drawButton(window, sf::FloatRect(bx + 10.f, y, 260.f, 20.f),
        m_config->dither == GradientDither::None ? "Dither: None" :
        m_config->dither == GradientDither::Bayer2x2 ? "Dither: Bayer 2x2" :
        m_config->dither == GradientDither::Bayer4x4 ? "Dither: Bayer 4x4" : "Dither: Bayer 8x8",
        sf::Color(50, 50, 60));
    y += 25.f;

    std::string bmStr = "Blend: ";
    if (m_config->blendModeIndex == 0) bmStr += "Replace";
    else if (m_config->blendModeIndex == 1) bmStr += "Normal";
    else if (m_config->blendModeIndex == 2) bmStr += "Multiply";
    else if (m_config->blendModeIndex == 3) bmStr += "Add";
    else bmStr += "Screen";
    drawButton(window, sf::FloatRect(bx + 10.f, y, 260.f, 20.f), bmStr, sf::Color(50, 50, 60));
    y += 25.f;

    drawButton(window, sf::FloatRect(bx + 10.f, y, 60.f, 20.f), "Opac-", sf::Color(60, 50, 50));
    drawButton(window, sf::FloatRect(bx + 75.f, y, 130.f, 20.f), "Opacity: " + std::to_string(static_cast<int>(m_config->opacity)) + "%", sf::Color(40, 40, 45));
    drawButton(window, sf::FloatRect(bx + 210.f, y, 60.f, 20.f), "Opac+", sf::Color(50, 60, 50));
    y += 25.f;

    drawToggle(window, sf::FloatRect(bx + 10.f, y, 80.f, 20.f), "Reverse", m_config->reverse);
    drawToggle(window, sf::FloatRect(bx + 95.f, y, 80.f, 20.f), "Repeat", m_config->repeat);
    drawToggle(window, sf::FloatRect(bx + 180.f, y, 90.f, 20.f), "Preview", m_config->livePreview);
    y += 25.f;

    drawToggle(window, sf::FloatRect(bx + 10.f, y, 260.f, 20.f), "Snap to Grid", m_config->snapToGrid);
    y += 35.f;

    m_gradientBar = sf::FloatRect(bx + 20.f, y, 240.f, 30.f);
    sf::RectangleShape barBg(sf::Vector2f(m_gradientBar.width, m_gradientBar.height));
    barBg.setPosition(m_gradientBar.left, m_gradientBar.top);
    barBg.setFillColor(sf::Color(50, 50, 50));
    window.draw(barBg);

    sf::VertexArray grad(sf::Quads);
    for (size_t i = 0; i < m_config->stops.size() - 1; ++i) {
        float x1 = m_gradientBar.left + m_config->stops[i].position * m_gradientBar.width;
        float x2 = m_gradientBar.left + m_config->stops[i + 1].position * m_gradientBar.width;
        sf::Color c1 = m_config->stops[i].color;
        sf::Color c2 = m_config->stops[i + 1].color;
        grad.append(sf::Vertex(sf::Vector2f(x1, m_gradientBar.top), c1));
        grad.append(sf::Vertex(sf::Vector2f(x2, m_gradientBar.top), c2));
        grad.append(sf::Vertex(sf::Vector2f(x2, m_gradientBar.top + m_gradientBar.height), c2));
        grad.append(sf::Vertex(sf::Vector2f(x1, m_gradientBar.top + m_gradientBar.height), c1));
    }
    window.draw(grad);

    for (size_t i = 0; i < m_config->stops.size(); ++i) {
        float sx = m_gradientBar.left + m_config->stops[i].position * m_gradientBar.width;
        sf::CircleShape stop(6.f);
        stop.setOrigin(6.f, 6.f);
        stop.setPosition(sx, m_gradientBar.top + m_gradientBar.height + 6.f);
        stop.setFillColor(m_config->stops[i].color);
        stop.setOutlineThickness(2.f);
        stop.setOutlineColor(static_cast<int>(i) == m_selectedStopIndex ? sf::Color::Yellow : sf::Color::White);
        window.draw(stop);
    }
}

bool GradientPanel::handleEvent(const sf::Event& event, sf::Vector2f mousePos) {
    if (!m_config) return false;

    float bx = m_background.getPosition().x;
    float y = m_background.getPosition().y + 35.f;

    static sf::Clock doubleClickClock;

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {

        bool isDoubleClick = (doubleClickClock.getElapsedTime().asMilliseconds() < 300);
        doubleClickClock.restart();

        if (isDoubleClick) {
            for (size_t i = 0; i < m_config->stops.size(); ++i) {
                float sx = m_gradientBar.left + m_config->stops[i].position * m_gradientBar.width;
                sf::FloatRect stopRect(sx - 8.f, m_gradientBar.top + m_gradientBar.height, 16.f, 16.f);
                if (stopRect.contains(mousePos)) {
                    m_selectedStopIndex = static_cast<int>(i);
                    m_requestColorPanelOpen = true;
                    return true;
                }
            }
        }

        if (!m_background.getGlobalBounds().contains(mousePos)) return false;

        if (sf::FloatRect(bx + 10.f, y, 260.f, 20.f).contains(mousePos)) {
            m_config->type = static_cast<GradientType>((static_cast<int>(m_config->type) + 1) % 5); return true;
        } y += 25.f;
        if (sf::FloatRect(bx + 10.f, y, 260.f, 20.f).contains(mousePos)) {
            m_config->interpolation = static_cast<GradientInterpolation>((static_cast<int>(m_config->interpolation) + 1) % 3); return true;
        } y += 25.f;
        if (sf::FloatRect(bx + 10.f, y, 260.f, 20.f).contains(mousePos)) {
            m_config->dither = static_cast<GradientDither>((static_cast<int>(m_config->dither) + 1) % 4); return true;
        } y += 25.f;
        if (sf::FloatRect(bx + 10.f, y, 260.f, 20.f).contains(mousePos)) {
            m_config->blendModeIndex = (m_config->blendModeIndex + 1) % 5;
            updateBlendMode();
            return true;
        } y += 25.f;
        if (sf::FloatRect(bx + 10.f, y, 60.f, 20.f).contains(mousePos)) {
            m_config->opacity = std::max(0.0f, m_config->opacity - 10.0f); return true;
        }
        if (sf::FloatRect(bx + 210.f, y, 60.f, 20.f).contains(mousePos)) {
            m_config->opacity = std::min(100.0f, m_config->opacity + 10.0f); return true;
        } y += 25.f;

        if (sf::FloatRect(bx + 10.f, y, 80.f, 20.f).contains(mousePos)) { m_config->reverse = !m_config->reverse; return true; }
        if (sf::FloatRect(bx + 95.f, y, 80.f, 20.f).contains(mousePos)) { m_config->repeat = !m_config->repeat; return true; }
        if (sf::FloatRect(bx + 180.f, y, 90.f, 20.f).contains(mousePos)) { m_config->livePreview = !m_config->livePreview; return true; }
        y += 25.f;

        if (sf::FloatRect(bx + 10.f, y, 260.f, 20.f).contains(mousePos)) { m_config->snapToGrid = !m_config->snapToGrid; return true; }

        for (size_t i = 0; i < m_config->stops.size(); ++i) {
            float sx = m_gradientBar.left + m_config->stops[i].position * m_gradientBar.width;
            sf::FloatRect stopRect(sx - 8.f, m_gradientBar.top + m_gradientBar.height, 16.f, 16.f);
            if (stopRect.contains(mousePos)) {
                m_selectedStopIndex = static_cast<int>(i);
                m_draggedStopIndex = static_cast<int>(i);
                return true;
            }
        }

        if (m_gradientBar.contains(mousePos)) {
            float pos = (mousePos.x - m_gradientBar.left) / m_gradientBar.width;
            m_config->stops.push_back({ pos, sf::Color::Black });
            sortStops();
            return true;
        }
    }
    else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        m_draggedStopIndex = -1;
    }
    else if (event.type == sf::Event::MouseMoved && m_draggedStopIndex != -1) {
        float pos = (mousePos.x - m_gradientBar.left) / m_gradientBar.width;
        m_config->stops[m_draggedStopIndex].position = std::clamp(pos, 0.0f, 1.0f);
        sortStops();
    }
    else if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Right) {
        for (size_t i = 0; i < m_config->stops.size(); ++i) {
            float sx = m_gradientBar.left + m_config->stops[i].position * m_gradientBar.width;
            sf::FloatRect stopRect(sx - 8.f, m_gradientBar.top + m_gradientBar.height, 16.f, 16.f);
            if (stopRect.contains(mousePos) && m_config->stops.size() > 2) {
                m_config->stops.erase(m_config->stops.begin() + i);
                m_selectedStopIndex = -1;
                return true;
            }
        }
    }
    return false;
}