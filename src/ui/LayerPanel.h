#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include "../core/Canvas.h"

enum class LayerPanelState { Hidden, Visible, Pinned };

class LayerPanel {
private:
    sf::RectangleShape background;
    sf::RectangleShape handleBg;
    sf::Text handleLabel;

    sf::RectangleShape pinBtn;
    sf::Text pinLabel;

    sf::RectangleShape addBtn;
    sf::RectangleShape dupBtn;
    sf::RectangleShape delBtn;

    sf::Font font;

    float width;
    float currentX;
    float targetX;
    LayerPanelState state;

public:
    LayerPanel();
    void init();
    void update(float dt, bool focusMode);
    void updateHover(sf::Vector2f mousePos);
    void draw(sf::RenderWindow& window, Canvas& canvas, int currentFrame);
    bool handleClick(sf::Vector2f mousePos, Canvas& canvas, int currentFrame);

    float getCurrentX() const;
    void forceClose();
    bool isHovered() const;
    bool isPanelPinned() const;
};