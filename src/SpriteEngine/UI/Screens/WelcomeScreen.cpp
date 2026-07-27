#include "WelcomeScreen.h"

WelcomeScreen::WelcomeScreen() {}

void WelcomeScreen::init() {
    font.loadFromFile("assets/font.otf");

    title.setFont(font);
    title.setString("WISDOM PARK STUDIO");
    title.setCharacterSize(60);
    title.setFillColor(sf::Color::White);
    title.setPosition(1920.f / 2.f - 300.f, 200.f);

    status.setFont(font);
    status.setCharacterSize(20);
    status.setPosition(20.f, 20.f);

    auto makeBtn = [&](std::string id, std::string text, float y) {
        WelcomeButton btn;
        btn.id = id;
        btn.rect.setSize(sf::Vector2f(400.f, 80.f));
        btn.rect.setPosition(1920.f / 2.f - 200.f, y);
        btn.rect.setFillColor(sf::Color(25, 25, 30, 230));
        btn.rect.setOutlineThickness(1.f);
        btn.rect.setOutlineColor(sf::Color(100, 100, 110, 100));

        btn.text.setFont(font);
        btn.text.setString(text);
        btn.text.setCharacterSize(30);
        btn.text.setFillColor(sf::Color::White);

        sf::FloatRect tRect = btn.text.getLocalBounds();
        btn.text.setOrigin(tRect.left + tRect.width / 2.0f, tRect.top + tRect.height / 2.0f);
        btn.text.setPosition(btn.rect.getPosition().x + 200.f, btn.rect.getPosition().y + 40.f);

        buttons.push_back(btn);
        };

    makeBtn("new_project", "New Animation Project", 400.f);
    makeBtn("config_ai", "Studio Settings", 520.f);
    makeBtn("exit", "Exit Software", 640.f);
}

void WelcomeScreen::updateHover(sf::Vector2f mousePos) {
    for (auto& btn : buttons) {
        btn.isHovered = btn.rect.getGlobalBounds().contains(mousePos);
    }
}

void WelcomeScreen::updateStatus(bool configured, const std::string& provider) {
    if (configured) {
        status.setString("AI Configured: " + provider);
        status.setFillColor(sf::Color(0, 191, 255));
    }
    else {
        status.setString("AI Not Configured");
        status.setFillColor(sf::Color(200, 50, 50));
    }
}

void WelcomeScreen::draw(sf::RenderWindow& window) {
    window.draw(title);
    window.draw(status);

    for (auto& btn : buttons) {
        if (btn.isHovered) {
            btn.rect.setFillColor(sf::Color(45, 45, 55, 250));
        }
        else {
            btn.rect.setFillColor(sf::Color(25, 25, 30, 230));
        }
        window.draw(btn.rect);
        window.draw(btn.text);
    }
}

std::string WelcomeScreen::handleClick(sf::Vector2f mousePos) {
    for (const auto& btn : buttons) {
        if (btn.rect.getGlobalBounds().contains(mousePos)) {
            return btn.id;
        }
    }
    return "";
}