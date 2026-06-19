#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

class ColorPalettePanel {
private:
    sf::RectangleShape background;
    sf::RectangleShape pinBtn;
    sf::Text pinLabel;

    sf::RectangleShape primaryBox;
    sf::RectangleShape secondaryBox;

    std::vector<sf::RectangleShape> swatches;
    sf::Font font;

    float width;
    float currentX;
    float targetX;
    bool isPinned;
    bool isHoveredAnywhere;

public:
    ColorPalettePanel();
    void init();
    void update(float dt, bool focusMode);
    void updateHover(sf::Vector2f mousePos);
    void draw(sf::RenderWindow& window);
    bool handleClick(sf::Vector2f mousePos, sf::Color& outPrimary, sf::Color& outSecondary);

    void setColors(sf::Color primary, sf::Color secondary);
    float getPanelLeftEdge() const;
};