#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <array>

enum class SelectionState {
    Inactive,
    Drawing,
    Selected,
    Floating
};

class SelectionManager {
private:
    SelectionState state;
    std::vector<sf::Vector2f> pathPoints;
    std::vector<sf::Vector2f> localPoints;
    sf::FloatRect boundingBox;
    sf::Texture floatingTexture;
    sf::Sprite floatingSprite;
    sf::Texture clipboardTexture;
    bool hasClipboard;
    sf::Texture dashTexture;
    float dashOffset;
    sf::Vector2f dragStartPos;
    bool isDragging;

    // Free-transform (resize by corner handles) state
    bool showHandles;
    float handleVisualSize;
    bool isResizingFlag;
    int activeHandle; // 0=TL, 1=TR, 2=BR, 3=BL, -1=none
    sf::Vector2f resizeAnchorWorld;
    sf::Vector2f resizeAnchorLocal;
    sf::Vector2f resizeDraggedLocal;

    bool isInsidePolygon(sf::Vector2f point, const std::vector<sf::Vector2f>& polygon) const;
    void calculateBoundingBox();
    void clampToCanvas(sf::Vector2u canvasSize, bool skip = false);

public:
    SelectionManager();
    void update(float dt);
    void draw(sf::RenderWindow& window, const sf::RenderStates& baseStates);
    void drawPixels(sf::RenderWindow& window, const sf::RenderStates& baseStates);
    void startLasso(sf::Vector2f pos, sf::Vector2u canvasSize);
    void addLassoPoint(sf::Vector2f pos, sf::Vector2u canvasSize);
    void endLasso();
    bool isPointInsideSelection(sf::Vector2f pos) const;
    void extractFromLayer(sf::RenderTexture* layerTexture, bool removeOriginal);
    void commitToLayer(sf::RenderTexture* layerTexture);
    void discardFloating();
    void clearSelection();
    void startDrag(sf::Vector2f pos);
    void drag(sf::Vector2f pos, sf::Vector2u canvasSize, bool allowOutsideCanvas = false);
    void endDrag();
    void copy();
    void paste(sf::Vector2u canvasSize);
    void deleteSelection(sf::RenderTexture* layerTexture);
    void flipHorizontal();
    void flipVertical();
    void duplicate(sf::RenderTexture* layerTexture, sf::Vector2u canvasSize);
    SelectionState getState() const;
    bool isActive() const;

    // Resize (free-transform corner handles) - only meaningful while Floating
    void setShowHandles(bool show);
    bool isShowingHandles() const;
    void setHandleVisualSize(float localSize);
    std::array<sf::Vector2f, 4> getHandlePositions() const; // world/canvas-space corners: TL,TR,BR,BL
    int hitTestHandle(sf::Vector2f pos, float handleRadius) const;
    bool startResize(sf::Vector2f pos, float handleRadius);
    void resize(sf::Vector2f pos, sf::Vector2u canvasSize, bool allowOutsideCanvas);
    void endResize();
    bool isResizing() const;
};