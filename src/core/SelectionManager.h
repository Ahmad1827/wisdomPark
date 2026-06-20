#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

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

    bool isInsidePolygon(sf::Vector2f point, const std::vector<sf::Vector2f>& polygon) const;
    void calculateBoundingBox();
    void clampToCanvas(sf::Vector2u canvasSize);

public:
    SelectionManager();

    void update(float dt);
    void draw(sf::RenderWindow& window, const sf::RenderStates& baseStates);

    void startLasso(sf::Vector2f pos, sf::Vector2u canvasSize);
    void addLassoPoint(sf::Vector2f pos, sf::Vector2u canvasSize);
    void endLasso();

    bool isPointInsideSelection(sf::Vector2f pos) const;

    void extractFromLayer(sf::RenderTexture* layerTexture, bool removeOriginal);
    void commitToLayer(sf::RenderTexture* layerTexture);
    void discardFloating();
    void clearSelection();

    void startDrag(sf::Vector2f pos);
    void drag(sf::Vector2f pos, sf::Vector2u canvasSize);
    void endDrag();

    void copy();
    void paste(sf::Vector2u canvasSize);
    void deleteSelection(sf::RenderTexture* layerTexture);

    void flipHorizontal();
    void flipVertical();
    void duplicate(sf::RenderTexture* layerTexture, sf::Vector2u canvasSize);

    SelectionState getState() const;
    bool isActive() const;
};