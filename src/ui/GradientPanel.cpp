#include "GradientPanel.h"
#include "../UI/UITheme.h"
#include <algorithm>
#include <cmath>

GradientPanel::GradientPanel() : m_config(nullptr), m_position(64.f, 78.f), m_size(280.f, 490.f), m_draggedStopIndex(-1), m_selectedStopIndex(-1) {}

void GradientPanel::init(GradientConfig* config) {
    m_config = config;
    m_font.loadFromFile("assets/font.otf");
    m_position = sf::Vector2f(64.f, 78.f);
}

void GradientPanel::update(float dt) {}

void GradientPanel::drawButton(sf::RenderWindow& window, sf::FloatRect bounds, const std::string& text, sf::Color bgColor) {
    bool hovered = bounds.contains(window.mapPixelToCoords(sf::Mouse::getPosition(window)));
    WisdomUI::Theme::DrawSunsetButton(window, bounds, text, m_font, 11, false, hovered, false, 1.0f);
}

void GradientPanel::drawToggle(sf::RenderWindow& window, sf::FloatRect bounds, const std::string& text, bool state) {
    bool hovered = bounds.contains(window.mapPixelToCoords(sf::Mouse::getPosition(window)));
    WisdomUI::Theme::DrawSunsetButton(window, bounds, text, m_font, 11, state, hovered, state, 1.0f);
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
    sf::FloatRect panelBounds(m_position.x, m_position.y, m_size.x, m_size.y);
    WisdomUI::Theme::DrawSunsetPanel(window, panelBounds, 1.0f);

    sf::FloatRect headerGrip(m_position.x + 8.f, m_position.y + 6.f, m_size.x - 16.f, 26.f);
    sf::RectangleShape gripBg(sf::Vector2f(headerGrip.width, headerGrip.height));
    gripBg.setPosition(headerGrip.left, headerGrip.top);
    gripBg.setFillColor(WisdomUI::Theme::SunsetDeepDark);
    gripBg.setOutlineThickness(1.f);
    gripBg.setOutlineColor(WisdomUI::Theme::SunsetPlum);
    window.draw(gripBg);

    WisdomUI::Theme::DrawCrispText(window, m_font, ":: GRADIENT TOOL ::", 12, headerGrip.left + headerGrip.width / 2.0f, headerGrip.top + headerGrip.height / 2.0f, WisdomUI::Theme::SunsetAmber, sf::Color(14, 6, 20), true, true);

    if (!m_config) return;

    float bx = m_position.x;
    float y = m_position.y + 40.f;

    drawButton(window, sf::FloatRect(bx + 12.f, y, 256.f, 24.f),
        m_config->type == GradientType::Linear ? "Type: Linear" :
        m_config->type == GradientType::Radial ? "Type: Radial" :
        m_config->type == GradientType::Diamond ? "Type: Diamond" :
        m_config->type == GradientType::Angle ? "Type: Angle" : "Type: Reflected",
        WisdomUI::Theme::SunsetSkyMid);
    y += 28.f;

    drawButton(window, sf::FloatRect(bx + 12.f, y, 256.f, 24.f),
        m_config->interpolation == GradientInterpolation::RGB ? "Interpolation: RGB" :
        m_config->interpolation == GradientInterpolation::HSV ? "Interpolation: HSV" : "Interpolation: Constant",
        WisdomUI::Theme::SunsetSkyMid);
    y += 28.f;

    drawButton(window, sf::FloatRect(bx + 12.f, y, 256.f, 24.f),
        m_config->dither == GradientDither::None ? "Dither: None" :
        m_config->dither == GradientDither::Bayer2x2 ? "Dither: Bayer 2x2" :
        m_config->dither == GradientDither::Bayer4x4 ? "Dither: Bayer 4x4" : "Dither: Bayer 8x8",
        WisdomUI::Theme::SunsetSkyMid);
    y += 28.f;

    std::string bmStr = "Blend: ";
    if (m_config->blendModeIndex == 0) bmStr += "Replace";
    else if (m_config->blendModeIndex == 1) bmStr += "Normal";
    else if (m_config->blendModeIndex == 2) bmStr += "Multiply";
    else if (m_config->blendModeIndex == 3) bmStr += "Add";
    else bmStr += "Screen";
    drawButton(window, sf::FloatRect(bx + 12.f, y, 256.f, 24.f), bmStr, WisdomUI::Theme::SunsetSkyMid);
    y += 28.f;

    drawButton(window, sf::FloatRect(bx + 12.f, y, 60.f, 24.f), "Opac-", WisdomUI::Theme::SunsetCoralDark);
    drawButton(window, sf::FloatRect(bx + 76.f, y, 128.f, 24.f), "Opacity: " + std::to_string(static_cast<int>(m_config->opacity)) + "%", WisdomUI::Theme::SunsetSkyTop);
    drawButton(window, sf::FloatRect(bx + 208.f, y, 60.f, 24.f), "Opac+", WisdomUI::Theme::SunsetCoralDark);
    y += 28.f;

    drawToggle(window, sf::FloatRect(bx + 12.f, y, 78.f, 24.f), "Reverse", m_config->reverse);
    drawToggle(window, sf::FloatRect(bx + 94.f, y, 78.f, 24.f), "Repeat", m_config->repeat);
    drawToggle(window, sf::FloatRect(bx + 176.f, y, 92.f, 24.f), "Preview", m_config->livePreview);
    y += 28.f;

    drawToggle(window, sf::FloatRect(bx + 12.f, y, 256.f, 24.f), "Snap to Grid", m_config->snapToGrid);
    y += 36.f;

    m_gradientBar = sf::FloatRect(bx + 16.f, y, 248.f, 28.f);
    sf::RectangleShape barBg(sf::Vector2f(m_gradientBar.width, m_gradientBar.height));
    barBg.setPosition(m_gradientBar.left, m_gradientBar.top);
    barBg.setFillColor(WisdomUI::Theme::SunsetDeepDark);
    barBg.setOutlineThickness(1.5f);
    barBg.setOutlineColor(WisdomUI::Theme::SunsetPlum);
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
        sf::CircleShape stop(7.f);
        stop.setOrigin(7.f, 7.f);
        stop.setPosition(sx, m_gradientBar.top + m_gradientBar.height + 7.f);
        stop.setFillColor(m_config->stops[i].color);
        stop.setOutlineThickness(2.f);
        stop.setOutlineColor(static_cast<int>(i) == m_selectedStopIndex ? WisdomUI::Theme::SunsetAmber : sf::Color::White);
        window.draw(stop);
    }
}

bool GradientPanel::handleEvent(const sf::Event& event, sf::Vector2f mousePos) {
    if (!m_config) return false;

    sf::FloatRect headerGrip(m_position.x, m_position.y, m_size.x, 34.f);

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        if (headerGrip.contains(mousePos)) {
            m_isDraggingPanel = true;
            m_dragOffset = mousePos - m_position;
            return true;
        }

        static sf::Clock doubleClickClock;
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

        if (!sf::FloatRect(m_position.x, m_position.y, m_size.x, m_size.y).contains(mousePos)) return false;

        float bx = m_position.x;
        float y = m_position.y + 40.f;

        if (sf::FloatRect(bx + 12.f, y, 256.f, 24.f).contains(mousePos)) {
            m_config->type = static_cast<GradientType>((static_cast<int>(m_config->type) + 1) % 5); return true;
        } y += 28.f;
        if (sf::FloatRect(bx + 12.f, y, 256.f, 24.f).contains(mousePos)) {
            m_config->interpolation = static_cast<GradientInterpolation>((static_cast<int>(m_config->interpolation) + 1) % 3); return true;
        } y += 28.f;
        if (sf::FloatRect(bx + 12.f, y, 256.f, 24.f).contains(mousePos)) {
            m_config->dither = static_cast<GradientDither>((static_cast<int>(m_config->dither) + 1) % 4); return true;
        } y += 28.f;
        if (sf::FloatRect(bx + 12.f, y, 256.f, 24.f).contains(mousePos)) {
            m_config->blendModeIndex = (m_config->blendModeIndex + 1) % 5;
            updateBlendMode();
            return true;
        } y += 28.f;
        if (sf::FloatRect(bx + 12.f, y, 60.f, 24.f).contains(mousePos)) {
            m_config->opacity = std::max(0.0f, m_config->opacity - 10.0f); return true;
        }
        if (sf::FloatRect(bx + 208.f, y, 60.f, 24.f).contains(mousePos)) {
            m_config->opacity = std::min(100.0f, m_config->opacity + 10.0f); return true;
        } y += 28.f;

        if (sf::FloatRect(bx + 12.f, y, 78.f, 24.f).contains(mousePos)) { m_config->reverse = !m_config->reverse; return true; }
        if (sf::FloatRect(bx + 94.f, y, 78.f, 24.f).contains(mousePos)) { m_config->repeat = !m_config->repeat; return true; }
        if (sf::FloatRect(bx + 176.f, y, 92.f, 24.f).contains(mousePos)) { m_config->livePreview = !m_config->livePreview; return true; }
        y += 28.f;

        if (sf::FloatRect(bx + 12.f, y, 256.f, 24.f).contains(mousePos)) { m_config->snapToGrid = !m_config->snapToGrid; return true; }

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
        m_isDraggingPanel = false;
        m_draggedStopIndex = -1;
    }
    else if (event.type == sf::Event::MouseMoved) {
        if (m_isDraggingPanel) {
            m_position = mousePos - m_dragOffset;
            m_position.x = std::clamp(m_position.x, 56.f, 1920.f - m_size.x);
            m_position.y = std::clamp(m_position.y, 40.f, 1080.f - m_size.y);
            return true;
        }
        if (m_draggedStopIndex != -1) {
            float pos = (mousePos.x - m_gradientBar.left) / m_gradientBar.width;
            m_config->stops[m_draggedStopIndex].position = std::clamp(pos, 0.0f, 1.0f);
            sortStops();
            return true;
        }
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
    return sf::FloatRect(m_position.x, m_position.y, m_size.x, m_size.y).contains(mousePos);
}