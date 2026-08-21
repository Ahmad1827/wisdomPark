#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include <string>
#include <vector>
#include "../core/AssetManager.h"
#include "../core/Canvas.h"

enum class BrowserView { Grid, List };

class AssetBrowserPanel {
private:
    AssetManager& assetManager;
    sf::Font font;
    AssetType currentCategory;
    BrowserView viewMode;
    std::string selectedAssetId;

    sf::Vector2f position;
    sf::Vector2f size;

    bool isDraggingPanel = false;
    sf::Vector2f dragOffset;

    bool isVisible = false;
    bool isDraggingAsset = false;
    sf::Vector2f dragStart;

    float scrollY = 0.0f;
    float maxScrollY = 0.0f;

    sf::FloatRect importBtnBounds;
    sf::FloatRect gridAreaBounds;
    std::vector<std::pair<sf::FloatRect, AssetType>> categoryBounds;
    std::vector<std::pair<sf::FloatRect, std::string>> deleteBtnBounds;

    void triggerImport();
    void handleDragAndDrop(const sf::Vector2f& dropPos, const sf::RenderWindow& window, Canvas& canvas, int currentFrame);

public:
    AssetBrowserPanel(AssetManager& am, const sf::Font& f);
    void toggle();
    bool getIsVisible() const;
    void setProject(const std::string& projPath);
    void setBounds(const sf::FloatRect& bounds);
    void update(float dt);
    void draw(sf::RenderWindow& window);
    void handleEvent(const sf::Event& event, const sf::RenderWindow& window, Canvas& canvas, int currentFrame);
};