#pragma once
#include <SFML/Graphics.hpp>
#include "../core/AssetManager.h"
#include "../core/Canvas.h"

enum class BrowserView { Grid, List };

class AssetBrowserPanel {
public:
    AssetBrowserPanel(AssetManager& am);
    void handleEvent(const sf::Event& event, const sf::RenderWindow& window, Canvas& canvas);
    void update(float dt);
    void draw(sf::RenderWindow& window);
    void setBounds(const sf::FloatRect& bounds);
    void setProject(const std::string& projPath);
    void toggle();
    bool getIsVisible() const;

private:
    AssetManager& assetManager;
    sf::FloatRect panelBounds;
    AssetType currentCategory;
    std::string searchQuery;
    BrowserView viewMode;
    AssetRecord* selectedAsset;
    float panelWidth;
    bool isCollapsed;
    bool isDragging;
    bool isResizing;
    bool isVisible;
    sf::Vector2f dragStart;

    sf::RectangleShape background;
    sf::RectangleShape topBar;
    sf::RectangleShape resizeHandle;

    void drawTopBar(sf::RenderWindow& window);
    void drawCategoryList(sf::RenderWindow& window);
    void drawAssetGrid(sf::RenderWindow& window);
    void drawProperties(sf::RenderWindow& window);
    void handleDragAndDrop(const sf::Vector2f& dropPos, const sf::RenderWindow& window, Canvas& canvas);
    void triggerImport();
};