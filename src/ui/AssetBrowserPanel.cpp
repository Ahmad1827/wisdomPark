#include "AssetBrowserPanel.h"
#include <iostream>

AssetBrowserPanel::AssetBrowserPanel(AssetManager& am)
    : assetManager(am), currentCategory(AssetType::Image), viewMode(BrowserView::Grid),
    selectedAsset(nullptr), panelWidth(300.f), isCollapsed(false), isDragging(false), isResizing(false), isVisible(false) {
    background.setFillColor(sf::Color(35, 35, 40));
    topBar.setFillColor(sf::Color(45, 45, 50));
    resizeHandle.setFillColor(sf::Color(60, 60, 65));
}

void AssetBrowserPanel::toggle() {
    isVisible = !isVisible;
}

bool AssetBrowserPanel::getIsVisible() const {
    return isVisible;
}

void AssetBrowserPanel::setProject(const std::string& projPath) {
    assetManager.init(projPath);
}

void AssetBrowserPanel::setBounds(const sf::FloatRect& bounds) {
    panelBounds = bounds;
    if (!isCollapsed) {
        panelBounds.width = panelWidth;
    }
    else {
        panelBounds.width = 30.f;
    }
    background.setPosition(panelBounds.left, panelBounds.top);
    background.setSize(sf::Vector2f(panelBounds.width, panelBounds.height));

    topBar.setPosition(panelBounds.left, panelBounds.top);
    topBar.setSize(sf::Vector2f(panelBounds.width, 40.f));

    resizeHandle.setPosition(panelBounds.left + panelBounds.width - 5.f, panelBounds.top);
    resizeHandle.setSize(sf::Vector2f(5.f, panelBounds.height));
}

void AssetBrowserPanel::handleEvent(const sf::Event& event, const sf::RenderWindow& window, Canvas& canvas) {
    if (!isVisible) return;

    if (event.type == sf::Event::MouseButtonPressed) {
        sf::Vector2f mousePos(static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y));
        if (resizeHandle.getGlobalBounds().contains(mousePos)) {
            isResizing = true;
        }
        else if (panelBounds.contains(mousePos)) {
            if (mousePos.y > panelBounds.top + 40.f && mousePos.y < panelBounds.top + panelBounds.height - 150.f) {
                isDragging = true;
                dragStart = mousePos;

                auto assets = assetManager.getAssetsByCategory(currentCategory);
                if (!assets.empty()) {
                    selectedAsset = assets[0];
                }
            }
        }
    }
    else if (event.type == sf::Event::MouseButtonReleased) {
        if (isResizing) isResizing = false;
        if (isDragging) {
            isDragging = false;
            sf::Vector2f mousePos(static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y));
            if (!panelBounds.contains(mousePos)) {
                handleDragAndDrop(mousePos, window, canvas);
            }
        }
    }
    else if (event.type == sf::Event::MouseMoved) {
        if (isResizing) {
            float newWidth = static_cast<float>(event.mouseMove.x) - panelBounds.left;
            if (newWidth > 100.f && newWidth < 800.f) {
                panelWidth = newWidth;
                setBounds(sf::FloatRect(panelBounds.left, panelBounds.top, panelWidth, panelBounds.height));
            }
        }
    }
}

void AssetBrowserPanel::handleDragAndDrop(const sf::Vector2f& dropPos, const sf::RenderWindow& window, Canvas& canvas) {
    if (!selectedAsset) return;

    if (selectedAsset->type == AssetType::Image) {
        canvas.importImageToActiveLayer(selectedAsset->filepath, 0);
    }
    else if (selectedAsset->type == AssetType::Audio) {
    }
    else if (selectedAsset->type == AssetType::Font) {
    }
}

void AssetBrowserPanel::update(float dt) {
    if (!isVisible) return;

    if (selectedAsset && !selectedAsset->thumbnailLoaded) {
        assetManager.requestThumbnail(selectedAsset);
    }
}

void AssetBrowserPanel::draw(sf::RenderWindow& window) {
    if (!isVisible) return;

    window.draw(background);
    if (!isCollapsed) {
        drawTopBar(window);
        drawCategoryList(window);
        drawAssetGrid(window);
        drawProperties(window);
    }
    window.draw(resizeHandle);
}

void AssetBrowserPanel::drawTopBar(sf::RenderWindow& window) {
    window.draw(topBar);
}

void AssetBrowserPanel::drawCategoryList(sf::RenderWindow& window) {
    sf::RectangleShape catArea(sf::Vector2f(100.f, panelBounds.height - 40.f));
    catArea.setPosition(panelBounds.left, panelBounds.top + 40.f);
    catArea.setFillColor(sf::Color(30, 30, 35));
    window.draw(catArea);
}

void AssetBrowserPanel::drawAssetGrid(sf::RenderWindow& window) {
    auto assets = assetManager.getAssetsByCategory(currentCategory);
    float startX = panelBounds.left + 110.f;
    float startY = panelBounds.top + 50.f;

    for (size_t i = 0; i < assets.size(); ++i) {
        sf::RectangleShape thumb(sf::Vector2f(64.f, 64.f));
        thumb.setPosition(startX + (i % 3) * 74.f, startY + (i / 3) * 74.f);
        if (assets[i]->thumbnailLoaded) {
            thumb.setTexture(&assets[i]->thumbnail);
        }
        else {
            thumb.setFillColor(sf::Color(80, 80, 80));
        }
        window.draw(thumb);
    }
}

void AssetBrowserPanel::drawProperties(sf::RenderWindow& window) {
    sf::RectangleShape propArea(sf::Vector2f(panelBounds.width, 150.f));
    propArea.setPosition(panelBounds.left, panelBounds.top + panelBounds.height - 150.f);
    propArea.setFillColor(sf::Color(25, 25, 30));
    window.draw(propArea);
}

void AssetBrowserPanel::triggerImport() {
}