#include "TopMenuBar.h"

TopMenuBar::TopMenuBar() {}

void TopMenuBar::init() {
    font.loadFromFile("assets/font.otf");

    background.setSize(sf::Vector2f(1920.f, 40.f));
    background.setPosition(0.f, 0.f);
    background.setFillColor(sf::Color(25, 25, 28, 240));

    auto makeMenu = [&](std::string id, std::string text, float x) {
        MenuButton btn;
        btn.id = id;
        btn.rect.setSize(sf::Vector2f(100.f, 40.f));
        btn.rect.setPosition(x, 0.f);

        btn.label.setFont(font);
        btn.label.setString(text);
        btn.label.setCharacterSize(16);

        sf::FloatRect tRect = btn.label.getLocalBounds();
        btn.label.setOrigin(tRect.left + tRect.width / 2.0f, tRect.top + tRect.height / 2.0f);
        btn.label.setPosition(btn.rect.getPosition().x + 50.f, btn.rect.getPosition().y + 20.f);

        buttons.push_back(btn);
        };

    makeMenu("file", "File", 0.f);
    makeMenu("edit", "Edit", 100.f);
    makeMenu("view", "View", 200.f);
    makeMenu("window", "Window", 300.f);
    makeMenu("settings", "Settings", 400.f);
    makeMenu("ai", "AI", 500.f);
}

void TopMenuBar::updateHover(sf::Vector2f mousePos) {
    for (auto& btn : buttons) {
        btn.isHovered = btn.rect.getGlobalBounds().contains(mousePos);
    }
}

void TopMenuBar::draw(sf::RenderWindow& window, bool isAIConfigured) {
    window.draw(background);
    for (auto& btn : buttons) {
        if (btn.id == "ai" && !isAIConfigured) {
            btn.rect.setFillColor(sf::Color::Transparent);
            btn.label.setFillColor(sf::Color(100, 100, 100));
        }
        else {
            btn.rect.setFillColor(btn.isHovered ? sf::Color(60, 60, 65) : sf::Color::Transparent);
            btn.label.setFillColor(sf::Color::White);
        }
        window.draw(btn.rect);
        window.draw(btn.label);
    }
}

std::string TopMenuBar::handleClick(sf::Vector2f mousePos) {
    for (const auto& btn : buttons) {
        if (btn.rect.getGlobalBounds().contains(mousePos)) {
            return btn.id;
        }
    }
    return "";
}