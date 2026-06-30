#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include "../core/Canvas.h"

class RightPanelManager {
private:
    sf::Font font;
    sf::RectangleShape background;
    sf::RectangleShape handleBg;
    sf::Text handleText;

    float currentX;
    float targetX;
    float width;
    bool pinned;
    bool hovered;

    sf::Color primaryColor;
    sf::Color secondaryColor;

    std::string activeTheme;
    bool isLightingMode;
    bool isTerrainEnabled;
    bool isOnionSkinEnabled;
    float onionOpacity;
    float timelineFps;

    sf::RectangleShape createBtn(sf::FloatRect bounds, sf::Color color);
    void drawText(sf::RenderWindow& window, const std::string& str, sf::Vector2f pos, int size, sf::Color col = sf::Color::White);

public:
    RightPanelManager();
    void init();

    void update(float dt, bool focusMode);
    void updateHover(sf::Vector2f mousePos);
    void draw(sf::RenderWindow& window, Canvas& canvas, int currentFrame);
    std::string handleClick(sf::Vector2f mousePos, Canvas& canvas, int currentFrame);
    bool handlePaletteClick(sf::Vector2f mousePos, sf::Color& outPrimary, sf::Color& outSecondary);

    void syncPropertiesState(std::string theme, bool lightMode, bool terrainOn, bool onionOn, float onionOp, float fps);

    bool isPanelPinned() const;
    void forceClose();
    bool isHovered() const;
    float getCurrentX() const;
    float getMinLeftEdge() const;

    // NEW: Expose Handle Bounds for precise UIManager hit testing
    sf::FloatRect getHandleBounds() const;
};