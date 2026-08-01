#include "AssetBrowserPanel.h"
#include "../core/NativeDialogs.h"
#include <iostream>

AssetBrowserPanel::AssetBrowserPanel(AssetManager& am, const sf::Font& f)
    : assetManager(am), font(f), currentCategory(AssetType::Image), viewMode(BrowserView::Grid),
    selectedAsset(nullptr), panelWidth(350.f), isCollapsed(false), isDragging(false), isResizing(false), isVisible(false) {
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
        else if (btnImportBounds.contains(mousePos)) {
            triggerImport();
        }
        else {
            for (const auto& cb : categoryBounds) {
                if (cb.first.contains(mousePos)) {
                    currentCategory = cb.second;
                    selectedAsset = nullptr;
                }
            }

            auto assets = assetManager.getAssetsByCategory(currentCategory);
            float startX = panelBounds.left + 110.f;
            float startY = panelBounds.top + 50.f;
            for (size_t i = 0; i < assets.size(); ++i) {
                float ax = startX + (i % 2) * 80.f;
                float ay = startY + (i / 2) * 100.f;
                if (sf::FloatRect(ax, ay, 64.f, 64.f).contains(mousePos)) {
                    selectedAsset = assets[i];
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
                handleDragAndDrop(mousePos, window, canvas);
            }
        }
    }
    else if (event.type == sf::Event::MouseMoved) {
        if (isResizing) {
            float newWidth = static_cast<float>(event.mouseMove.x) - panelBounds.left;
            if (newWidth > 200.f && newWidth < 800.f) {
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

    sf::Text title("Asset Browser", font, 16);
    title.setPosition(panelBounds.left + 10.f, panelBounds.top + 10.f);
    title.setFillColor(sf::Color::White);
    window.draw(title);

    btnImportBounds = sf::FloatRect(panelBounds.left + panelBounds.width - 70.f, panelBounds.top + 7.f, 60.f, 26.f);
    sf::RectangleShape importBtn(sf::Vector2f(btnImportBounds.width, btnImportBounds.height));
    importBtn.setPosition(btnImportBounds.left, btnImportBounds.top);
    importBtn.setFillColor(sf::Color(0, 150, 100));
    window.draw(importBtn);

    sf::Text importTxt("Import", font, 14);
    importTxt.setPosition(btnImportBounds.left + 8.f, btnImportBounds.top + 3.f);
    importTxt.setFillColor(sf::Color::White);
    window.draw(importTxt);
}

void AssetBrowserPanel::drawCategoryList(sf::RenderWindow& window) {
    sf::RectangleShape catArea(sf::Vector2f(100.f, panelBounds.height - 40.f));
    catArea.setPosition(panelBounds.left, panelBounds.top + 40.f);
    catArea.setFillColor(sf::Color(30, 30, 35));
    window.draw(catArea);

    categoryBounds.clear();
    std::vector<std::pair<std::string, AssetType>> cats = {
        {"Images", AssetType::Image},
        {"Audio", AssetType::Audio},
        {"Fonts", AssetType::Font},
        {"Brushes", AssetType::Brush},
        {"Patterns", AssetType::Pattern},
        {"AI Assets", AssetType::AI}
    };

    float y = panelBounds.top + 50.f;
    for (const auto& cat : cats) {
        sf::Text t(cat.first, font, 14);
        t.setPosition(panelBounds.left + 10.f, y);
        t.setFillColor(currentCategory == cat.second ? sf::Color(0, 191, 255) : sf::Color(150, 150, 150));
        window.draw(t);
        categoryBounds.push_back({ sf::FloatRect(panelBounds.left, y, 100.f, 25.f), cat.second });
        y += 30.f;
    }
}

void AssetBrowserPanel::drawAssetGrid(sf::RenderWindow& window) {
    auto assets = assetManager.getAssetsByCategory(currentCategory);
    float startX = panelBounds.left + 110.f;
    float startY = panelBounds.top + 50.f;

    for (size_t i = 0; i < assets.size(); ++i) {
        float ax = startX + (i % 2) * 80.f;
        float ay = startY + (i / 2) * 100.f;

        sf::RectangleShape thumb(sf::Vector2f(64.f, 64.f));
        thumb.setPosition(ax, ay);

        if (assets[i] == selectedAsset) {
            thumb.setOutlineThickness(2.f);
            thumb.setOutlineColor(sf::Color(0, 191, 255));
        }
        else {
            thumb.setOutlineThickness(0.f);
        }

        if (assets[i]->thumbnailLoaded) {
            thumb.setTexture(&assets[i]->thumbnail);
        }
        else {
            thumb.setFillColor(sf::Color(80, 80, 80));
        }
        window.draw(thumb);

        sf::Text nameTxt(assets[i]->filename, font, 10);
        nameTxt.setPosition(ax, ay + 68.f);
        if (nameTxt.getLocalBounds().width > 70.f) {
            std::string trunc = assets[i]->filename.substr(0, 8) + "...";
            nameTxt.setString(trunc);
        }
        nameTxt.setFillColor(sf::Color::White);
        window.draw(nameTxt);
    }
}

void AssetBrowserPanel::drawProperties(sf::RenderWindow& window) {
    sf::RectangleShape propArea(sf::Vector2f(panelBounds.width, 150.f));
    propArea.setPosition(panelBounds.left, panelBounds.top + panelBounds.height - 150.f);
    propArea.setFillColor(sf::Color(25, 25, 30));
    window.draw(propArea);

    if (selectedAsset) {
        sf::Text propTxt("Name: " + selectedAsset->filename + "\n\nType: " + selectedAsset->extension + "\n\nSize: " + std::to_string(selectedAsset->fileSize / 1024) + " KB", font, 14);
        propTxt.setPosition(panelBounds.left + 10.f, panelBounds.top + panelBounds.height - 140.f);
        propTxt.setFillColor(sf::Color::White);
        window.draw(propTxt);
    }
}

void AssetBrowserPanel::triggerImport() {
    std::string file = NativeDialogs::openFileDialog("Supported Files\0*.png;*.jpg;*.jpeg;*.bmp;*.wav;*.ogg;*.ttf\0All Files\0*.*\0");
    if (!file.empty()) {
        assetManager.importAssets({ file });
    }
}