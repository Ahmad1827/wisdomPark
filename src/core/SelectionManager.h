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
    bool isLassoSelection{ true };
    std::vector<sf::Vector2f> pathPoints;
    std::vector<sf::Vector2f> localPoints;
    sf::FloatRect boundingBox;
    std::vector<sf::FloatRect> subItemBoxes;

    sf::Texture floatingTexture;
    sf::Sprite floatingSprite;
    sf::Texture clipboardTexture;
    bool hasClipboard;
    sf::Texture dashTexture;
    float dashOffset;
    sf::Vector2f dragStartPos;
    bool m_isDragging;

    bool showHandles;
    float handleVisualSize;
    bool isResizingFlag;
    int activeHandle;
    sf::Vector2f resizeAnchorWorld;
    sf::Vector2f resizeAnchorLocal;
    sf::Vector2f resizeDraggedLocal;
    sf::FloatRect resizeStartBox;

    bool isInsidePolygon(sf::Vector2f point, const std::vector<sf::Vector2f>& polygon) const;
    void calculateBoundingBox();
    void clampToCanvas(sf::Vector2u canvasSize, bool skip = false);
    sf::Vector2f clipboardPos;
    int pasteCount = 0;

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
    bool isDragging() const { return m_isDragging; }

    void copy(sf::RenderTexture* layerTexture);
    void moveFloating(sf::Vector2f offset, sf::Vector2u canvasSize);
    void paste(sf::Vector2u canvasSize);
    void deleteSelection(sf::RenderTexture* layerTexture);
    void flipHorizontal();
    void flipVertical();
    void duplicate(sf::RenderTexture* layerTexture, sf::Vector2u canvasSize);

    SelectionState getState() const { return state; }
    void setState(SelectionState s) { state = s; }
    bool isActive() const { return state != SelectionState::Inactive; }
    sf::FloatRect getBoundingBox() const { return boundingBox; }
    void setBoundingBox(const sf::FloatRect& box) { boundingBox = box; }
    sf::Transform getFloatingTransform() const { return floatingSprite.getTransform(); }

    void setLassoMode(bool isLasso) { isLassoSelection = isLasso; }
    bool getIsLassoMode() const { return isLassoSelection; }

    const std::vector<sf::Vector2f>& getPathPoints() const { return pathPoints; }
    void setPathPoints(const std::vector<sf::Vector2f>& pts) { pathPoints = pts; }

    void setSelectionBoxes(const sf::FloatRect& masterBox, const std::vector<sf::FloatRect>& itemBoxes);
    const std::vector<sf::FloatRect>& getSubItemBoxes() const { return subItemBoxes; }
    void setSubItemBoxes(const std::vector<sf::FloatRect>& boxes) { subItemBoxes = boxes; }

    void moveSelection(sf::Vector2f delta);
    void flipPathHorizontal(float midX);
    void flipPathVertical(float midY);

    void setShowHandles(bool show);
    bool isShowingHandles() const { return showHandles; }
    void setHandleVisualSize(float localSize);
    std::array<sf::Vector2f, 4> getHandlePositions() const;
    int hitTestHandle(sf::Vector2f pos, float handleRadius) const;
    bool startResize(sf::Vector2f pos, float handleRadius);
    void resize(sf::Vector2f pos, sf::Vector2u canvasSize, bool allowOutsideCanvas = true);
    void endResize();
    bool isResizing() const { return isResizingFlag; }
};