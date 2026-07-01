#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

enum class RightPanelState { Hidden, Visible, Pinned };

struct PropItem {
    std::string id;
    sf::RectangleShape rect;
    sf::Text label;
    bool isHovered = false;
    bool isActive = false;
};

struct PropSection {
    std::string id;
    sf::RectangleShape headerRect;
    sf::Text headerLabel;
    std::vector<PropItem> items;
    bool isOpen = true;
    bool isHovered = false;
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
    bool hovered;
    bool pinned;

    void updateLayout();

public:
    RightProperties();
    void init();
    void update(float dt, bool focusMode);
    void updateHover(sf::Vector2f mousePos, bool canOpen);
    void draw(sf::RenderWindow& window);
    std::string handleClick(sf::Vector2f mousePos);
    void syncState(const std::string& theme, bool lighting, bool terrain, bool onion, float onionOpacity, float currentFps);

    float getCurrentX() const;
    void forceClose();
    bool isHovered() const;
    bool isPanelPinned() const;
    sf::FloatRect getHandleBounds() const;
};