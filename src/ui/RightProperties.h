#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

enum class RightPanelState { Hidden, Visible, Pinned };

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
    sf::RectangleShape handleBg;
    sf::Text handleLabel;

    sf::RectangleShape pinBtn;
    sf::Text pinLabel;

    std::vector<PropSection> sections;
    sf::Font font;

    float width;
    float currentX;
    float targetX;
    RightPanelState state;

    void updateLayout();

public:
    RightProperties();
    void init();
    void update(float dt, bool focusMode);
    void updateHover(sf::Vector2f mousePos);
    void draw(sf::RenderWindow& window);
    std::string handleClick(sf::Vector2f mousePos);
    void syncState(const std::string& theme, bool lighting, bool terrain, bool onion, float onionOpacity);

    float getCurrentX() const;
    void forceClose();
    bool isHovered() const;
    bool isPanelPinned() const;
};