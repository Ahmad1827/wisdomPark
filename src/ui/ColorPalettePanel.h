#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include "../core/Canvas.h"

enum class PalettePanelState { Hidden, Visible, Pinned };

class ColorPalettePanel {
private:
    sf::RectangleShape background;
    sf::RectangleShape handleBg;
    sf::Text handleLabel;

    sf::RectangleShape pinBtn;
    sf::Text pinLabel;

    sf::RectangleShape primaryBox;
    sf::RectangleShape secondaryBox;

    std::vector<sf::RectangleShape> swatches;
    sf::Font font;

    float width;
    float currentX;
    float targetX;
    PalettePanelState state;

public:
    ColorPalettePanel();
    void init();
    void update(float dt, bool focusMode);
    void updateHover(sf::Vector2f mousePos, bool canOpen);
    void draw(sf::RenderWindow& window);

    bool handleClick(sf::Vector2f mousePos, Canvas& canvas);
    void setColors(sf::Color primary, sf::Color secondary);

    float getCurrentX() const;
    void forceClose();
    bool isHovered() const;
    bool isPanelPinned() const;
    bool isOpen() const;
    sf::FloatRect getHandleBounds() const;
};