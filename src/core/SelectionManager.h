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
    std::vector<sf::Vector2f> localPoints; // Stored in local space for transforms

    sf::FloatRect boundingBox;
    sf::Texture floatingTexture;
    sf::Sprite floatingSprite;

    sf::Texture clipboardTexture;
    bool hasClipboard;

    sf::Texture dashTexture;
    float dashOffset;

    sf::Vector2f dragStartPos;

    bool isInsidePolygon(sf::Vector2f point, const std::vector<sf::Vector2f>& polygon) const;
    void calculateBoundingBox();

public:
    SelectionManager();

    void update(float dt);
    void draw(sf::RenderWindow& window, const sf::RenderStates& baseStates);

    void startLasso(sf::Vector2f pos);
    void addLassoPoint(sf::Vector2f pos);
    void endLasso();

    bool isPointInsideSelection(sf::Vector2f pos) const;

    void cutFromLayer(sf::RenderTexture* layerTexture);
    void commitToLayer(sf::RenderTexture* layerTexture);
    void discardFloating();
    void clearSelection();

    void startDrag(sf::Vector2f pos);
    void drag(sf::Vector2f pos);

    void copy();
    void paste();
    void deleteSelection(sf::RenderTexture* layerTexture);

    SelectionState getState() const;
    bool isActive() const;
};