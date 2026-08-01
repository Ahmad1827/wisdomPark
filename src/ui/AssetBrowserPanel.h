#pragma once
#include <SFML/Graphics.hpp>
#include "../core/AssetManager.h"
#include "../core/Canvas.h"

enum class BrowserView { Grid, List };

class AssetBrowserPanel {
public:
    AssetBrowserPanel(AssetManager& am, const sf::Font& font);
    void handleEvent(const sf::Event& event, const sf::RenderWindow& window, Canvas& canvas, int currentFrame);
    void update(float dt);
    void draw(sf::RenderWindow& window);
    void setBounds(const sf::FloatRect& bounds);
    void setProject(const std::string& projPath);
    void toggle();
    bool getIsVisible() const;
    float getWidth() const;

private:
    AssetManager& assetManager;
    const sf::Font& font;
    sf::FloatRect panelBounds;
    AssetType currentCategory;
    std::string searchQuery;
    BrowserView viewMode;
    std::string selectedAssetId;
    float panelWidth;
    bool isCollapsed;
    bool isDragging;
    bool isResizing;
    bool isVisible;
    float animProgress; // Controls the fade & slide
    sf::Vector2f dragStart;

    sf::RectangleShape background;
    sf::RectangleShape resizeHandle;

    sf::FloatRect btnImportBounds;
    std::vector<std::pair<sf::FloatRect, AssetType>> categoryBounds;

    void drawTopBar(sf::RenderWindow& window, float currentX, sf::Uint8 alpha);
    void drawCategoryList(sf::RenderWindow& window, float currentX, sf::Uint8 alpha);
    void drawAssetGrid(sf::RenderWindow& window, float currentX, sf::Uint8 alpha);
    void drawProperties(sf::RenderWindow& window, float currentX, sf::Uint8 alpha);
    void handleDragAndDrop(const sf::Vector2f& dropPos, const sf::RenderWindow& window, Canvas& canvas, int currentFrame);
    void triggerImport();

    sf::Color applyAlpha(sf::Color color, sf::Uint8 alpha) const;
};