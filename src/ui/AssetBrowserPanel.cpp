#include "AssetBrowserPanel.h"
#include "../core/NativeDialogs.h"
#include <iostream>

AssetBrowserPanel::AssetBrowserPanel(AssetManager& am, const sf::Font& f)
    : assetManager(am), font(f), currentCategory(AssetType::Image), viewMode(BrowserView::Grid),
    selectedAssetId(""), panelWidth(260.f), isCollapsed(false), isDragging(false), isResizing(false), isVisible(false) {
    background.setFillColor(sf::Color(20, 20, 25, 240));
    background.setOutlineThickness(1.f);
    background.setOutlineColor(sf::Color(255, 255, 255, 15));
    topBar.setFillColor(sf::Color(30, 30, 35, 255));
    resizeHandle.setFillColor(sf::Color(60, 60, 65, 100));
}

void AssetBrowserPanel::toggle() {
    isVisible = !isVisible;
}

bool AssetBrowserPanel::getIsVisible() const {
    return isVisible;
}

float AssetBrowserPanel::getWidth() const {
    return panelWidth;
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
    topBar.setSize(sf::Vector2f(panelBounds.width, 35.f));

    resizeHandle.setPosition(panelBounds.left + panelBounds.width - 5.f, panelBounds.top);
    resizeHandle.setSize(sf::Vector2f(5.f, panelBounds.height));
}

void AssetBrowserPanel::handleEvent(const sf::Event& event, const sf::RenderWindow& window, Canvas& canvas, int currentFrame) {
    if (!isVisible) return;

    if (event.type == sf::Event::MouseButtonPressed) {
        sf::Vector2f mousePos(static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y));
        if (resizeHandle.getGlobalBounds().contains(mousePos)) {
            isResizing = true;
        }
        else if (btnImportBounds.contains(mousePos)) {
            triggerImport();
        }
        else {
            for (const auto& cb : categoryBounds) {
                if (cb.first.contains(mousePos)) {
                    currentCategory = cb.second;
                    selectedAssetId = "";
                }
            }

            auto assets = assetManager.getAssetsByCategory(currentCategory);
            float startX = panelBounds.left + 85.f;
            float startY = panelBounds.top + 45.f;
            for (size_t i = 0; i < assets.size(); ++i) {
                float ax = startX + (i % 2) * 75.f;
                float ay = startY + (i / 2) * 90.f;
                if (sf::FloatRect(ax, ay, 60.f, 60.f).contains(mousePos)) {
                    selectedAssetId = assets[i]->id;
                    isDragging = true;
                    dragStart = mousePos;
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
                handleDragAndDrop(mousePos, window, canvas, currentFrame);
            }
        }
    }
    else if (event.type == sf::Event::MouseMoved) {
        if (isResizing) {
            float newWidth = static_cast<float>(event.mouseMove.x) - panelBounds.left;
            if (newWidth > 180.f && newWidth < 800.f) {
                panelWidth = newWidth;
                setBounds(sf::FloatRect(panelBounds.left, panelBounds.top, panelWidth, panelBounds.height));
            }
        }
    }
}

void AssetBrowserPanel::handleDragAndDrop(const sf::Vector2f& dropPos, const sf::RenderWindow& window, Canvas& canvas, int currentFrame) {
    AssetRecord* selectedAsset = assetManager.getAsset(selectedAssetId);
    if (!selectedAsset) return;

    if (selectedAsset->type == AssetType::Image) {
        canvas.importImageToActiveLayer(selectedAsset->filepath, currentFrame);
    }
    else if (selectedAsset->type == AssetType::Audio) {
    }
    else if (selectedAsset->type == AssetType::Font) {
    }
}

void AssetBrowserPanel::update(float dt) {
    if (!isVisible) return;

    AssetRecord* selectedAsset = assetManager.getAsset(selectedAssetId);
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

    sf::Text title("ASSETS", font, 12);
    title.setPosition(panelBounds.left + 15.f, panelBounds.top + 10.f);
    title.setFillColor(sf::Color(200, 200, 200));
    window.draw(title);

    btnImportBounds = sf::FloatRect(panelBounds.left + panelBounds.width - 65.f, panelBounds.top + 5.f, 55.f, 24.f);
    sf::RectangleShape importBtn(sf::Vector2f(btnImportBounds.width, btnImportBounds.height));
    importBtn.setPosition(btnImportBounds.left, btnImportBounds.top);
    importBtn.setFillColor(sf::Color(0, 122, 204));
    importBtn.setOutlineThickness(1.f);
    importBtn.setOutlineColor(sf::Color(255, 255, 255, 50));
    window.draw(importBtn);

    sf::Text importTxt("Import", font, 10);
    importTxt.setPosition(btnImportBounds.left + 10.f, btnImportBounds.top + 5.f);
    importTxt.setFillColor(sf::Color::White);
    window.draw(importTxt);
}

void AssetBrowserPanel::drawCategoryList(sf::RenderWindow& window) {
    sf::RectangleShape catArea(sf::Vector2f(75.f, panelBounds.height - 35.f));
    catArea.setPosition(panelBounds.left, panelBounds.top + 35.f);
    catArea.setFillColor(sf::Color(15, 15, 18, 150));
    window.draw(catArea);

    categoryBounds.clear();
    std::vector<std::pair<std::string, AssetType>> cats = {
        {"Images", AssetType::Image},
        {"Audio", AssetType::Audio},
        {"Fonts", AssetType::Font},
        {"Brushes", AssetType::Brush},
        {"Patterns", AssetType::Pattern},
        {"AI", AssetType::AI}
    };

    float y = panelBounds.top + 45.f;
    for (const auto& cat : cats) {
        sf::Text t(cat.first, font, 10);
        t.setPosition(panelBounds.left + 10.f, y);
        t.setFillColor(currentCategory == cat.second ? sf::Color(0, 191, 255) : sf::Color(150, 150, 150));
        window.draw(t);
        categoryBounds.push_back({ sf::FloatRect(panelBounds.left, y, 75.f, 25.f), cat.second });
        y += 25.f;
    }
}

void AssetBrowserPanel::drawAssetGrid(sf::RenderWindow& window) {
    auto assets = assetManager.getAssetsByCategory(currentCategory);
    float startX = panelBounds.left + 85.f;
    float startY = panelBounds.top + 45.f;

    for (size_t i = 0; i < assets.size(); ++i) {
        float ax = startX + (i % 2) * 75.f;
        float ay = startY + (i / 2) * 90.f;

        sf::RectangleShape thumb(sf::Vector2f(60.f, 60.f));
        thumb.setPosition(ax, ay);

        if (assets[i]->id == selectedAssetId) {
            thumb.setOutlineThickness(1.f);
            thumb.setOutlineColor(sf::Color(0, 191, 255));
        }
        else {
            thumb.setOutlineThickness(1.f);
            thumb.setOutlineColor(sf::Color(255, 255, 255, 20));
        }

        if (assets[i]->thumbnailLoaded) {
            thumb.setTexture(&assets[i]->thumbnail);
        }
        else {
            thumb.setFillColor(sf::Color(40, 40, 45));
        }
        window.draw(thumb);

        sf::Text nameTxt(assets[i]->filename, font, 9);
        nameTxt.setPosition(ax, ay + 64.f);
        if (nameTxt.getLocalBounds().width > 65.f) {
            std::string trunc = assets[i]->filename.substr(0, 8) + "...";
            nameTxt.setString(trunc);
        }
        nameTxt.setFillColor(sf::Color(200, 200, 200));
        window.draw(nameTxt);
    }
}

void AssetBrowserPanel::drawProperties(sf::RenderWindow& window) {
    sf::RectangleShape propArea(sf::Vector2f(panelBounds.width, 100.f));
    propArea.setPosition(panelBounds.left, panelBounds.top + panelBounds.height - 100.f);
    propArea.setFillColor(sf::Color(15, 15, 18, 200));
    propArea.setOutlineThickness(1.f);
    propArea.setOutlineColor(sf::Color(255, 255, 255, 15));
    window.draw(propArea);

    AssetRecord* selectedAsset = assetManager.getAsset(selectedAssetId);
    if (selectedAsset) {
        sf::Text propTxt("File: " + selectedAsset->filename + "\nType: " + selectedAsset->extension + "\nSize: " + std::to_string(selectedAsset->fileSize / 1024) + " KB", font, 10);
        propTxt.setPosition(panelBounds.left + 10.f, panelBounds.top + panelBounds.height - 90.f);
        propTxt.setFillColor(sf::Color(180, 180, 180));
        propTxt.setLineSpacing(1.5f);
        window.draw(propTxt);
    }
}