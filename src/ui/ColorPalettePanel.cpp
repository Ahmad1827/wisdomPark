#include "ColorPalettePanel.h"

ColorPalettePanel::ColorPalettePanel() : width(200.f), currentX(1920.f), targetX(1920.f), isPinned(false), isHoveredAnywhere(false) {}

void ColorPalettePanel::init() {
    font.loadFromFile("assets/font.otf");

    background.setSize(sf::Vector2f(width, 400.f));
    background.setFillColor(sf::Color(15, 15, 18, 220));
    background.setOutlineThickness(1.f);
    background.setOutlineColor(sf::Color(255, 255, 255, 15));

    pinBtn.setSize(sf::Vector2f(width - 20.f, 24.f));
    pinBtn.setFillColor(sf::Color(255, 255, 255, 10));

    pinLabel.setFont(font);
    pinLabel.setString("Color Palette");
    pinLabel.setCharacterSize(12);
    pinLabel.setFillColor(sf::Color(180, 180, 180));

    primaryBox.setSize(sf::Vector2f(60.f, 60.f));
    primaryBox.setOutlineThickness(2.f);
    primaryBox.setOutlineColor(sf::Color::White);

    secondaryBox.setSize(sf::Vector2f(60.f, 60.f));
    secondaryBox.setOutlineThickness(1.f);
    secondaryBox.setOutlineColor(sf::Color(100, 100, 100));

    // Aseprite-inspired default palette
    std::vector<sf::Color> defaultColors = {
        sf::Color(0, 0, 0), sf::Color(255, 255, 255), sf::Color(157, 157, 157),
        sf::Color(255, 0, 68), sf::Color(250, 166, 19), sf::Color(255, 219, 0),
        sf::Color(146, 224, 0), sf::Color(26, 186, 0), sf::Color(26, 203, 208),
        sf::Color(0, 114, 213), sf::Color(111, 48, 218), sf::Color(204, 76, 250)
    };

    for (auto& c : defaultColors) {
        sf::RectangleShape swatch(sf::Vector2f(30.f, 30.f));
        swatch.setFillColor(c);
        swatches.push_back(swatch);
    }
}

void ColorPalettePanel::update(float dt, bool focusMode) {
    if (focusMode) targetX = 1920.f + 20.f;
    else targetX = (isPinned || isHoveredAnywhere) ? 1920.f - width - 10.f : 1920.f + 20.f;

    currentX += (targetX - currentX) * 15.0f * dt;
    background.setPosition(currentX, 600.f);

    pinBtn.setPosition(currentX + 10.f, 610.f);
    pinLabel.setPosition(currentX + 20.f, 614.f);

    primaryBox.setPosition(currentX + 30.f, 650.f);
    secondaryBox.setPosition(currentX + 100.f, 670.f);

    float sx = currentX + 15.f;
    float sy = 750.f;
    for (size_t i = 0; i < swatches.size(); ++i) {
        swatches[i].setPosition(sx, sy);
        sx += 35.f;
        if (sx > currentX + width - 30.f) { sx = currentX + 15.f; sy += 35.f; }
    }
}

void ColorPalettePanel::updateHover(sf::Vector2f mousePos) {
    isHoveredAnywhere = mousePos.x > (currentX - 20.f) && mousePos.y > 580.f;
}

void ColorPalettePanel::draw(sf::RenderWindow& window) {
    window.draw(background);
    window.draw(pinBtn);
    window.draw(pinLabel);

    window.draw(secondaryBox); // Draw behind
    window.draw(primaryBox);

    for (auto& s : swatches) window.draw(s);
}

bool ColorPalettePanel::handleClick(sf::Vector2f mousePos, sf::Color& outPrimary, sf::Color& outSecondary) {
    if (pinBtn.getGlobalBounds().contains(mousePos)) {
        isPinned = !isPinned; return true;
    }

    for (auto& s : swatches) {
        if (s.getGlobalBounds().contains(mousePos)) {
            if (sf::Mouse::isButtonPressed(sf::Mouse::Right)) {
                outSecondary = s.getFillColor();
                secondaryBox.setFillColor(outSecondary);
            }
            else {
                outPrimary = s.getFillColor();
                primaryBox.setFillColor(outPrimary);
            }
            return true;
        }
    }
    return false;
}

void ColorPalettePanel::setColors(sf::Color primary, sf::Color secondary) {
    primaryBox.setFillColor(primary);
    secondaryBox.setFillColor(secondary);
}
float ColorPalettePanel::getPanelLeftEdge() const { return currentX; }