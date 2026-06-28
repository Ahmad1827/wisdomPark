#pragma once
#include <SFML/Graphics.hpp>
#include "../core/Timeline.h"
#include "../core/Canvas.h"
#include <vector>
#include <string>

enum class BottomPanelState { Hidden, Visible, Pinned };

struct TimelineButton {
    sf::RectangleShape rect;
    sf::Text label;
    std::string id;
    bool isHovered = false;
};

class BottomTimeline {
private:
    sf::RectangleShape background;
    sf::RectangleShape handleBg;
    sf::Text handleLabel;

    sf::RectangleShape pinBtn;
    sf::Text pinLabel;

    std::vector<TimelineButton> buttons;
    sf::Font font;

    float height;
    float currentY;
    float targetY;
    BottomPanelState state;

    bool onionEnabled;
    int onionPrev;
    int onionNext;

public:
    BottomTimeline();
    void init();
    void update(float dt, bool focusMode);
    void updateHover(sf::Vector2f mousePos);
    void draw(sf::RenderWindow& window, Timeline& timeline, Canvas& canvas);
    std::string handleClick(sf::Vector2f mousePos);
    int handleFrameClick(sf::Vector2f mousePos, size_t frameCount);

    void syncOnionState(bool enabled, int prevCount, int nextCount);
    float getPanelTopEdge() const;
};