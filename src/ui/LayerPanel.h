#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include "../core/Canvas.h"

enum class LayerPanelState { Hidden, Visible, Pinned };

class LayerPanel {
private:
    sf::Font font;
    sf::RectangleShape background;
    sf::RectangleShape headerBg;
    sf::Text headerText;

    sf::RectangleShape closeBtn;
    sf::Text closeText;
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
    bool isDraggingScrollbar;
    float scrollDragStartY;
    float scrollDragStartOffset;

    int renamingLayerIndex;
    std::string renameBuffer;
    sf::RectangleShape renameBox;
    sf::Text renameText;

    int draggedLayerIndex;
    int dropVisualSlot;
    sf::Vector2f dragCurrentPos;
    bool isDragging;

    int activeOpacityIndex;
    bool isDraggingOpacity;

    sf::Clock clickTimer;
    int lastClickedLayerIndex;

    float currentX;
    float targetX;
    float width;
    LayerPanelState state;

    struct LayerRow {
        sf::FloatRect bounds;
        sf::FloatRect colorTagBounds;
        sf::FloatRect eyeBounds;
        sf::FloatRect lockBounds;
        sf::FloatRect persistBounds;
        sf::FloatRect thumbBounds;
        sf::FloatRect nameBounds;
        sf::FloatRect opacityBounds;
        sf::FloatRect blendBounds;
    };
    std::vector<LayerRow> rowCache;

    sf::Color getTagColor(int tagId) const;
    void renderEyeIcon(sf::RenderWindow& window, sf::FloatRect bounds, bool visible);
    void renderLockIcon(sf::RenderWindow& window, sf::FloatRect bounds, bool locked);
    void renderPersistIcon(sf::RenderWindow& window, sf::FloatRect bounds, bool persistent);

public:
    LayerPanel();
    void init();
    void update(float dt, bool focusMode, bool isOpen = true);
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