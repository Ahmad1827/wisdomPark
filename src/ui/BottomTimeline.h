#pragma once
#include <SFML/Graphics.hpp>
#include "../core/Timeline.h"
#include "../core/Canvas.h"
#include <vector>
#include <string>

struct TimelineButton {
    sf::RectangleShape rect;
    sf::Text label;
    std::string id;
    bool isHovered = false;
};

class BottomTimeline {
private:
    sf::RectangleShape background;
    std::vector<TimelineButton> buttons;
    sf::Font font;
    float scrollOffset;

public:
    BottomTimeline();
    void init();
    void updateHover(sf::Vector2f mousePos);
    void draw(sf::RenderWindow& window, Timeline& timeline, Canvas& canvas);
    std::string handleClick(sf::Vector2f mousePos);
    int handleFrameClick(sf::Vector2f mousePos, size_t frameCount);
};