#include "ColorPalettePanel.h"

ColorPalettePanel::ColorPalettePanel() : width(200.f), currentX(1920.f), targetX(1920.f), state(PalettePanelState::Hidden) {}

void ColorPalettePanel::init() {
    font.loadFromFile("assets/font.otf");

    background.setSize(sf::Vector2f(width, 400.f));
    background.setFillColor(sf::Color(15, 15, 18, 220));
    background.setOutlineThickness(1.f);
    background.setOutlineColor(sf::Color(255, 255, 255, 15));

    handleBg.setSize(sf::Vector2f(24.f, 80.f));
    handleBg.setFillColor(sf::Color(30, 30, 35, 200));
    handleBg.setOutlineThickness(1.f);
    handleBg.setOutlineColor(sf::Color(255, 255, 255, 30));

    handleLabel.setFont(font);
    handleLabel.setString("<");
    handleLabel.setCharacterSize(16);
    handleLabel.setFillColor(sf::Color(200, 200, 200));

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
    if (focusMode) targetX = 1920.f;
    else targetX = (state == PalettePanelState::Pinned || state == PalettePanelState::Visible) ? 1920.f - width : 1920.f;

    currentX += (targetX - currentX) * 15.0f * dt;
    background.setPosition(currentX, 600.f);

    handleBg.setPosition(currentX - 24.f, 650.f);
    handleLabel.setPosition(currentX - 18.f, 680.f);

    if (state == PalettePanelState::Pinned) handleLabel.setString("x");
    else handleLabel.setString("<");

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
    bool inPanel = background.getGlobalBounds().contains(mousePos);
    bool inHandle = handleBg.getGlobalBounds().contains(mousePos);

    if (state == PalettePanelState::Hidden && inHandle) state = PalettePanelState::Visible;
    else if (state == PalettePanelState::Visible && !inPanel && !inHandle) state = PalettePanelState::Hidden;
}

void ColorPalettePanel::draw(sf::RenderWindow& window) {
    window.draw(background);
    if (state != PalettePanelState::Pinned) {
        window.draw(handleBg);
        window.draw(handleLabel);
    }
    window.draw(pinBtn);
    window.draw(pinLabel);

    window.draw(secondaryBox);
    window.draw(primaryBox);

    for (auto& s : swatches) window.draw(s);
}

bool ColorPalettePanel::handleClick(sf::Vector2f mousePos, sf::Color& outPrimary, sf::Color& outSecondary) {
    if (pinBtn.getGlobalBounds().contains(mousePos)) {
        state = (state == PalettePanelState::Pinned) ? PalettePanelState::Visible : PalettePanelState::Pinned;
        return true;
    }

    if (state == PalettePanelState::Hidden && handleBg.getGlobalBounds().contains(mousePos)) {
        state = PalettePanelState::Pinned;
        return true;
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
    primaryBox.setFillColor(primary); secondaryBox.setFillColor(secondary);
}

float ColorPalettePanel::getCurrentX() const { return currentX; }
void ColorPalettePanel::forceClose() { if (state != PalettePanelState::Pinned) state = PalettePanelState::Hidden; }
bool ColorPalettePanel::isHovered() const { return state == PalettePanelState::Visible; }
bool ColorPalettePanel::isPanelPinned() const { return state == PalettePanelState::Pinned; }