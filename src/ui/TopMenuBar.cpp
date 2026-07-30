#include "TopMenuBar.h"

TopMenuBar::TopMenuBar() {}

void TopMenuBar::init(float windowWidth) {
    m_font.loadFromFile("assets/font.otf");

    float barWidth = 480.f;
    float barHeight = 32.f;
    float startX = (windowWidth - barWidth) / 2.f;
    float startY = 8.f;

    m_background.setSize(sf::Vector2f(barWidth, barHeight));
    m_background.setPosition(startX, startY);
    m_background.setFillColor(sf::Color(110, 75, 45));
    m_background.setOutlineThickness(2.f);
    m_background.setOutlineColor(sf::Color(65, 40, 25));

    m_leftCap.setRadius(barHeight / 2.f);
    m_leftCap.setPosition(startX - m_leftCap.getRadius(), startY);
    m_leftCap.setFillColor(sf::Color(110, 75, 45));
    m_leftCap.setOutlineThickness(2.f);
    m_leftCap.setOutlineColor(sf::Color(65, 40, 25));

    m_rightCap.setRadius(barHeight / 2.f);
    m_rightCap.setPosition(startX + barWidth - m_rightCap.getRadius(), startY);
    m_rightCap.setFillColor(sf::Color(110, 75, 45));
    m_rightCap.setOutlineThickness(2.f);
    m_rightCap.setOutlineColor(sf::Color(65, 40, 25));

    m_buttons.clear();
    std::vector<std::pair<std::string, std::string>> btnData = {
        {"file", "File"},
        {"brushes", "Brushes"},
        {"colors", "Colors"},
        {"layers", "Layers"},
        {"save", "Save"}
    };

    float currentX = startX + 15.f;
    for (const auto& data : btnData) {
        TopMenuButton btn;
        btn.id = data.first;
        btn.label = data.second;
        btn.bounds = sf::FloatRect(currentX, startY, 90.f, barHeight);
        m_buttons.push_back(btn);
        currentX += 90.f;
    }
}

void TopMenuBar::resize(float windowWidth) {
    float barWidth = 480.f;
    float startX = (windowWidth - barWidth) / 2.f;
    float startY = 8.f;

    m_background.setPosition(startX, startY);
    m_leftCap.setPosition(startX - m_leftCap.getRadius(), startY);
    m_rightCap.setPosition(startX + barWidth - m_rightCap.getRadius(), startY);

    float currentX = startX + 15.f;
    for (auto& btn : m_buttons) {
        btn.bounds.left = currentX;
        currentX += 90.f;
    }
}

void TopMenuBar::update(sf::Vector2f mousePos) {
    for (auto& btn : m_buttons) {
        btn.isHovered = btn.bounds.contains(mousePos);
    }
}

void TopMenuBar::draw(sf::RenderWindow& window) {
    window.draw(m_background);
    window.draw(m_leftCap);
    window.draw(m_rightCap);

    sf::RectangleShape coverLeft(sf::Vector2f(4.f, m_background.getSize().y - 4.f));
    coverLeft.setPosition(m_background.getPosition().x, m_background.getPosition().y + 2.f);
    coverLeft.setFillColor(sf::Color(110, 75, 45));
    window.draw(coverLeft);

    sf::RectangleShape coverRight(sf::Vector2f(4.f, m_background.getSize().y - 4.f));
    coverRight.setPosition(m_background.getPosition().x + m_background.getSize().x - 4.f, m_background.getPosition().y + 2.f);
    coverRight.setFillColor(sf::Color(110, 75, 45));
    window.draw(coverRight);

    for (const auto& btn : m_buttons) {
        if (btn.isHovered) {
            sf::RectangleShape hoverBg(sf::Vector2f(btn.bounds.width - 10.f, btn.bounds.height - 8.f));
            hoverBg.setPosition(btn.bounds.left + 5.f, btn.bounds.top + 4.f);
            hoverBg.setFillColor(sf::Color(255, 255, 255, 30));
            hoverBg.setOutlineThickness(1.f);
            hoverBg.setOutlineColor(sf::Color(255, 255, 255, 50));
            window.draw(hoverBg);
        }

        sf::Text text(btn.label, m_font, 14);
        sf::FloatRect tBounds = text.getLocalBounds();
        text.setPosition(
            btn.bounds.left + (btn.bounds.width - tBounds.width) / 2.f,
            btn.bounds.top + (btn.bounds.height - text.getCharacterSize()) / 2.f - 2.f
        );
        text.setFillColor(sf::Color(240, 220, 180));
        window.draw(text);
    }
}

std::string TopMenuBar::handleEvent(const sf::Event& event, sf::Vector2f mousePos) {
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        for (const auto& btn : m_buttons) {
            if (btn.bounds.contains(mousePos)) {
                return btn.id;
            }
        }
    }
    return "";
}