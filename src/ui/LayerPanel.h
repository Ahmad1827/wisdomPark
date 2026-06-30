#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include "../core/Canvas.h"

enum class LayerPanelState { Hidden, Visible, Pinned };

class LayerPanel {
private:
    sf::Font font;
    sf::RectangleShape background;
    sf::RectangleShape handleBg;
    sf::Text handleText;
    sf::RectangleShape headerBg;
    sf::Text headerText;

    sf::RectangleShape pinBtn;
    sf::Text pinText;

    sf::RectangleShape addBtn;
    sf::Text addText;
    sf::RectangleShape dupBtn;
    sf::Text dupText;
    sf::RectangleShape delBtn;
    sf::Text delText;
    sf::RectangleShape mergeDownBtn;
    sf::Text mergeDownText;
    sf::RectangleShape mergeVisBtn;
    sf::Text mergeVisText;
    sf::RectangleShape pushBtn;
    sf::Text pushText;

    float scrollOffset;
    float maxScroll;

    int renamingLayerIndex;
    std::string renameBuffer;
    sf::RectangleShape renameBox;
    sf::Text renameText;

    int draggedLayerIndex;
    sf::Vector2f dragStartPos;
    bool isDragging;

    float currentX;
    float targetX;
    float width;
    LayerPanelState state;

    struct LayerRow {
        sf::FloatRect bounds;
        sf::FloatRect eyeBounds;
        sf::FloatRect lockBounds;
        sf::FloatRect persistBounds;
        sf::FloatRect colorTagBounds;
        sf::FloatRect nameBounds;
        sf::FloatRect opacityBounds;
        sf::FloatRect blendBounds;
    };
    std::vector<LayerRow> rowCache;

    sf::Color getTagColor(int tagId) const;

public:
    LayerPanel();
    void init();
    void update(float dt, bool focusMode);
    void updateHover(sf::Vector2f mousePos, bool canOpen);
    void draw(sf::RenderWindow& window, Canvas& canvas, int currentFrame);
    std::string processClick(sf::Vector2f mousePos, Canvas& canvas, int currentFrame);

    bool handleClick(sf::Vector2f mousePos, Canvas& canvas, int currentFrame);
    bool handleEvent(const sf::Event& event, sf::Vector2f mousePos, Canvas& canvas, int currentFrame);

    bool isPanelPinned() const;
    void forceClose();
    bool isHovered() const;
    float getCurrentX() const;
    bool isOpen() const;
    sf::FloatRect getHandleBounds() const;
};