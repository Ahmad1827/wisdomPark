#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

struct MenuButton {
    sf::RectangleShape rect;
    sf::Text label;
    std::string id;
    bool isHovered = false;
};

class TopMenuBar {
private:
    sf::RectangleShape background;
    std::vector<MenuButton> buttons;
    sf::Font font;

public:
    TopMenuBar();
    void init();
    void updateHover(sf::Vector2f mousePos);
    void draw(sf::RenderWindow& window, bool isAIConfigured);
    std::string handleClick(sf::Vector2f mousePos);
};