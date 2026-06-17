#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

struct PropItem {
    sf::RectangleShape rect;
    sf::Text label;
    std::string id;
    bool isHovered = false;
    bool isActive = false;
};

struct PropSection {
    sf::RectangleShape headerRect;
    sf::Text headerLabel;
    std::string id;
    bool isOpen = false;
    bool isHovered = false;
    std::vector<PropItem> items;
};

class RightProperties {
private:
    sf::RectangleShape background;
    std::vector<PropSection> sections;
    sf::Font font;

    void updateLayout();

public:
    RightProperties();
    void init();
    void updateHover(sf::Vector2f mousePos);
    void draw(sf::RenderWindow& window);
    std::string handleClick(sf::Vector2f mousePos);
    void syncState(const std::string& theme, bool lighting, bool terrain, bool onion, float onionOpacity);
};