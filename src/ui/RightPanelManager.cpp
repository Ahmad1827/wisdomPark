#include "RightPanelManager.h"
#include <algorithm>
#include <iostream>

RightPanelManager::RightPanelManager() : currentX(1920.f), targetX(1920.f), width(300.f), pinned(false), hovered(false), primaryColor(sf::Color::Black), secondaryColor(sf::Color::White), activeTheme("all"), isLightingMode(false), isTerrainEnabled(false), isOnionSkinEnabled(false), onionOpacity(89.25f), timelineFps(12.f) {}

void RightPanelManager::init() {
    font.loadFromFile("assets/font.otf");

    background.setFillColor(sf::Color(20, 20, 25, 240));
    background.setOutlineThickness(1.f);
    background.setOutlineColor(sf::Color(255, 255, 255, 30));

    handleBg.setFillColor(sf::Color(40, 40, 45, 200));
    handleBg.setOutlineThickness(1.f);
    handleBg.setOutlineColor(sf::Color(255, 255, 255, 30));

    handleText.setFont(font);
    handleText.setString("< P");
    handleText.setCharacterSize(14);
    handleText.setFillColor(sf::Color(200, 200, 200));
}

void RightPanelManager::syncPropertiesState(std::string theme, bool lightMode, bool terrainOn, bool onionOn, float onionOp, float fps) {
    activeTheme = theme;
    isLightingMode = lightMode;
    isTerrainEnabled = terrainOn;
    isOnionSkinEnabled = onionOn;
    onionOpacity = onionOp;
    timelineFps = fps;
}

void RightPanelManager::update(float dt, bool focusMode) {
    if (focusMode) {
        targetX = 1920.f;
    }
    else {
        if (pinned || hovered) targetX = 1920.f - width;
        else targetX = 1920.f;
    }

    currentX += (targetX - currentX) * 15.f * dt;

    background.setPosition(currentX, 0.f);
    background.setSize(sf::Vector2f(width, 1080.f));

    handleBg.setPosition(currentX - 30.f, 200.f);
    handleBg.setSize(sf::Vector2f(30.f, 80.f));
    handleText.setPosition(currentX - 25.f, 230.f);
}

void RightPanelManager::updateHover(sf::Vector2f mousePos) {
    if (currentX >= 1919.f) {
        hovered = handleBg.getGlobalBounds().contains(mousePos);
    }
    else {
        hovered = background.getGlobalBounds().contains(mousePos) || handleBg.getGlobalBounds().contains(mousePos);
    }
}

bool RightPanelManager::isPanelPinned() const { return pinned; }
void RightPanelManager::forceClose() { if (!pinned) targetX = 1920.f; hovered = false; }
bool RightPanelManager::isHovered() const { return hovered; }
float RightPanelManager::getCurrentX() const { return currentX; }
float RightPanelManager::getMinLeftEdge() const { return currentX; }
sf::FloatRect RightPanelManager::getHandleBounds() const { return handleBg.getGlobalBounds(); }

sf::RectangleShape RightPanelManager::createBtn(sf::FloatRect bounds, sf::Color color) {
    sf::RectangleShape r(sf::Vector2f(bounds.width, bounds.height));
    r.setPosition(bounds.left, bounds.top);
    r.setFillColor(color);
    return r;
}

void RightPanelManager::drawText(sf::RenderWindow& window, const std::string& str, sf::Vector2f pos, int size, sf::Color col) {
    sf::Text t(str, font, size);
    t.setPosition(pos);
    t.setFillColor(col);
    window.draw(t);
}

void RightPanelManager::draw(sf::RenderWindow& window, Canvas& canvas, int currentFrame) {
    if (currentX > 1919.f) return;

    window.draw(background);
    if (!pinned) {
        window.draw(handleBg);
        window.draw(handleText);
    }

    sf::RectangleShape headerBg(sf::Vector2f(width, 30.f));
    headerBg.setPosition(currentX, 0.f);
    headerBg.setFillColor(sf::Color(40, 40, 45, 255));
    window.draw(headerBg);
    drawText(window, "PROPERTIES", sf::Vector2f(currentX + 10.f, 5.f), 14, sf::Color(200, 200, 200));

    sf::RectangleShape pinBtn = createBtn(sf::FloatRect(currentX + 10.f, 40.f, 60.f, 24.f), sf::Color(50, 50, 60));
    if (pinned) {
        pinBtn.setOutlineThickness(1.f);
        pinBtn.setOutlineColor(sf::Color(0, 191, 255));
    }
    window.draw(pinBtn);
    drawText(window, pinned ? "Unpin" : "Pin", sf::Vector2f(currentX + 25.f, 43.f), 12, pinned ? sf::Color(0, 191, 255) : sf::Color::White);

    float y = 90.f;
    drawText(window, "AI Theme Settings", sf::Vector2f(currentX + 10.f, y), 12, sf::Color(150, 150, 150));
    y += 20.f;

    auto drawThemeBtn = [&](std::string id, std::string label, float bx, float by, float bw) {
        sf::RectangleShape btn = createBtn(sf::FloatRect(bx, by, bw, 24.f), activeTheme == id ? sf::Color(0, 122, 204, 180) : sf::Color(50, 50, 60));
        window.draw(btn);
        drawText(window, label, sf::Vector2f(bx + 10.f, by + 3.f), 12);
        };
    drawThemeBtn("all", "All", currentX + 10.f, y, 60.f);
    drawThemeBtn("structure", "Struct", currentX + 80.f, y, 60.f);
    drawThemeBtn("clutter", "Clutter", currentX + 150.f, y, 60.f);
    drawThemeBtn("wfc", "WFC", currentX + 220.f, y, 50.f);

    y += 40.f;
    sf::RectangleShape lightBtn = createBtn(sf::FloatRect(currentX + 10.f, y, 130.f, 24.f), isLightingMode ? sf::Color(255, 200, 0, 100) : sf::Color(50, 50, 60));
    window.draw(lightBtn);
    drawText(window, "Lighting Mode", sf::Vector2f(currentX + 30.f, y + 3.f), 12);

    sf::RectangleShape terrBtn = createBtn(sf::FloatRect(currentX + 150.f, y, 130.f, 24.f), isTerrainEnabled ? sf::Color(50, 200, 50, 100) : sf::Color(50, 50, 60));
    window.draw(terrBtn);
    drawText(window, "Terrain Rules", sf::Vector2f(currentX + 175.f, y + 3.f), 12);

    y += 40.f;
    sf::RectangleShape div1(sf::Vector2f(width - 20.f, 1.f)); div1.setPosition(currentX + 10.f, y); div1.setFillColor(sf::Color(255, 255, 255, 20)); window.draw(div1);
    y += 15.f;

    drawText(window, "Colors", sf::Vector2f(currentX + 10.f, y), 12, sf::Color(150, 150, 150));
    y += 20.f;

    sf::RectangleShape pBox(sf::Vector2f(40.f, 40.f)); pBox.setPosition(currentX + 10.f, y); pBox.setFillColor(canvas.getPrimaryColor()); pBox.setOutlineThickness(1.f); pBox.setOutlineColor(sf::Color::White); window.draw(pBox);
    sf::RectangleShape sBox(sf::Vector2f(40.f, 40.f)); sBox.setPosition(currentX + 30.f, y + 20.f); sBox.setFillColor(canvas.getSecondaryColor()); sBox.setOutlineThickness(1.f); sBox.setOutlineColor(sf::Color::White); window.draw(sBox);

    float px = currentX + 80.f;
    std::vector<sf::Color> pal = { sf::Color::Black, sf::Color::White, sf::Color::Red, sf::Color::Green, sf::Color::Blue, sf::Color::Yellow, sf::Color(255, 128, 0), sf::Color(128, 0, 255), sf::Color(0, 255, 255), sf::Color(255, 0, 255), sf::Color(128, 128, 128), sf::Color(139, 69, 19) };
    for (int i = 0; i < 12; ++i) {
        sf::RectangleShape cBox(sf::Vector2f(20.f, 20.f));
        cBox.setPosition(px + (i % 6) * 25.f, y + (i / 6) * 25.f);
        cBox.setFillColor(pal[i]);
        window.draw(cBox);
    }
}

std::string RightPanelManager::handleClick(sf::Vector2f mousePos, Canvas& canvas, int currentFrame) {
    if (sf::FloatRect(currentX + 10.f, 40.f, 60.f, 24.f).contains(mousePos)) { pinned = !pinned; return "pin_toggle"; }

    float y = 110.f;
    if (sf::FloatRect(currentX + 10.f, y, 60.f, 24.f).contains(mousePos)) return "theme_all";
    if (sf::FloatRect(currentX + 80.f, y, 60.f, 24.f).contains(mousePos)) return "theme_struct";
    if (sf::FloatRect(currentX + 150.f, y, 60.f, 24.f).contains(mousePos)) return "theme_clutter";
    if (sf::FloatRect(currentX + 220.f, y, 50.f, 24.f).contains(mousePos)) return "theme_wfc";

    y += 40.f;
    if (sf::FloatRect(currentX + 10.f, y, 130.f, 24.f).contains(mousePos)) return "toggle_light";
    if (sf::FloatRect(currentX + 150.f, y, 130.f, 24.f).contains(mousePos)) return "toggle_terrain";

    return "";
}

bool RightPanelManager::handlePaletteClick(sf::Vector2f mousePos, sf::Color& outPrimary, sf::Color& outSecondary) {
    if (currentX > 1919.f) return false;
    float px = currentX + 80.f;
    float y = 185.f;
    std::vector<sf::Color> pal = { sf::Color::Black, sf::Color::White, sf::Color::Red, sf::Color::Green, sf::Color::Blue, sf::Color::Yellow, sf::Color(255, 128, 0), sf::Color(128, 0, 255), sf::Color(0, 255, 255), sf::Color(255, 0, 255), sf::Color(128, 128, 128), sf::Color(139, 69, 19) };
    for (int i = 0; i < 12; ++i) {
        if (sf::FloatRect(px + (i % 6) * 25.f, y + (i / 6) * 25.f, 20.f, 20.f).contains(mousePos)) {
            outPrimary = pal[i];
            outSecondary = pal[i];
            return true;
        }
    }
    return false;
}