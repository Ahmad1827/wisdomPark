#include "AssetBrowserPanel.h"
#include "../core/NativeDialogs.h"
#include <iostream>
#include <algorithm>

AssetBrowserPanel::AssetBrowserPanel(AssetManager& am, const sf::Font& f)
    : assetManager(am), font(f), currentCategory(AssetType::Image), viewMode(BrowserView::Grid),
    selectedAssetId(""), panelWidth(280.f), isCollapsed(false), isDragging(false), isResizing(false), isVisible(false), animProgress(0.f) {
    background.setOutlineThickness(1.f);
}

void AssetBrowserPanel::toggle() {
    isVisible = !isVisible;
}

bool AssetBrowserPanel::getIsVisible() const {
    return isVisible;
}

float AssetBrowserPanel::getWidth() const {
    return panelWidth; // We keep bounds allocated even when fading out to push the UI smoothly
}

void AssetBrowserPanel::setProject(const std::string& projPath) {
    assetManager.init(projPath);
}

sf::Color AssetBrowserPanel::applyAlpha(sf::Color color, sf::Uint8 alpha) const {
    return sf::Color(color.r, color.g, color.b, static_cast<sf::Uint8>((color.a * alpha) / 255));
}

void AssetBrowserPanel::setBounds(const sf::FloatRect& bounds) {
    panelBounds = bounds;
    if (!isCollapsed) panelBounds.width = panelWidth;
    else panelBounds.width = 30.f;
}

void AssetBrowserPanel::update(float dt) {
    if (isVisible) {
        animProgress += 12.0f * dt;
        if (animProgress > 1.0f) animProgress = 1.0f;
    }
    else {
        animProgress -= 12.0f * dt;
        if (animProgress < 0.0f) animProgress = 0.0f;
    }

    if (animProgress > 0.0f) {
        AssetRecord* selectedAsset = assetManager.getAsset(selectedAssetId);
        if (selectedAsset && !selectedAsset->thumbnailLoaded) {
            assetManager.requestThumbnail(selectedAsset);
        }
    }
}

void AssetBrowserPanel::handleEvent(const sf::Event& event, const sf::RenderWindow& window, Canvas& canvas, int currentFrame) {
    if (animProgress < 0.5f) return; // Ignore clicks if mostly hidden

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
            float startX = panelBounds.left + 90.f;
            float startY = panelBounds.top + 55.f;
            for (size_t i = 0; i < assets.size(); ++i) {
                float ax = startX + (i % 2) * 85.f;
                float ay = startY + (i / 2) * 105.f;
                if (sf::FloatRect(ax, ay, 75.f, 75.f).contains(mousePos)) {
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
            if (newWidth > 220.f && newWidth < 800.f) {
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
}


void AssetBrowserPanel::draw(sf::RenderWindow& window) {
    if (animProgress <= 0.0f) return;

    // Slide-in and fade calculation
    float currentX = panelBounds.left - 25.f * (1.f - animProgress);
    sf::Uint8 alpha = static_cast<sf::Uint8>(animProgress * 255);

    background.setPosition(currentX, panelBounds.top);
    background.setSize(sf::Vector2f(panelBounds.width, panelBounds.height));
    background.setFillColor(applyAlpha(sf::Color(18, 18, 22, 245), alpha));
    background.setOutlineColor(applyAlpha(sf::Color(255, 255, 255, 15), alpha));

    resizeHandle.setPosition(currentX + panelBounds.width - 5.f, panelBounds.top);
    resizeHandle.setSize(sf::Vector2f(5.f, panelBounds.height));
    resizeHandle.setFillColor(applyAlpha(sf::Color(255, 255, 255, 5), alpha));

    window.draw(background);
    if (!isCollapsed) {
        drawTopBar(window, currentX, alpha);
        drawCategoryList(window, currentX, alpha);
        drawAssetGrid(window, currentX, alpha);
        drawProperties(window, currentX, alpha);
    }
    window.draw(resizeHandle);
}

void AssetBrowserPanel::drawTopBar(sf::RenderWindow& window, float currentX, sf::Uint8 alpha) {
    sf::RectangleShape topBar(sf::Vector2f(panelBounds.width, 40.f));
    topBar.setPosition(currentX, panelBounds.top);
    topBar.setFillColor(applyAlpha(sf::Color(25, 25, 30, 255), alpha));
    window.draw(topBar);

    sf::Text title("ASSET BROWSER", font, 11);
    title.setPosition(currentX + 15.f, panelBounds.top + 14.f);
    title.setFillColor(applyAlpha(sf::Color(200, 200, 210), alpha));
    title.setLetterSpacing(1.5f);
    window.draw(title);

    btnImportBounds = sf::FloatRect(currentX + panelBounds.width - 70.f, panelBounds.top + 8.f, 60.f, 24.f);
    sf::RectangleShape importBtn(sf::Vector2f(btnImportBounds.width, btnImportBounds.height));
    importBtn.setPosition(btnImportBounds.left, btnImportBounds.top);
    importBtn.setFillColor(applyAlpha(sf::Color(0, 122, 204, 200), alpha));
    importBtn.setOutlineThickness(1.f);
    importBtn.setOutlineColor(applyAlpha(sf::Color(0, 191, 255, 100), alpha));
    window.draw(importBtn);

    sf::Text importTxt("Import", font, 10);
    importTxt.setPosition(btnImportBounds.left + 14.f, btnImportBounds.top + 5.f);
    importTxt.setFillColor(applyAlpha(sf::Color::White, alpha));
    window.draw(importTxt);
}

void AssetBrowserPanel::drawCategoryList(sf::RenderWindow& window, float currentX, sf::Uint8 alpha) {
    sf::RectangleShape catArea(sf::Vector2f(80.f, panelBounds.height - 40.f));
    catArea.setPosition(currentX, panelBounds.top + 40.f);
    catArea.setFillColor(applyAlpha(sf::Color(12, 12, 15, 180), alpha));
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

    float y = panelBounds.top + 55.f;
    for (const auto& cat : cats) {
        bool isSelected = (currentCategory == cat.second);

        if (isSelected) {
            sf::RectangleShape highlight(sf::Vector2f(76.f, 26.f));
            highlight.setPosition(currentX + 2.f, y - 4.f);
            highlight.setFillColor(applyAlpha(sf::Color(0, 122, 204, 80), alpha));
            highlight.setOutlineThickness(1.f);
            highlight.setOutlineColor(applyAlpha(sf::Color(0, 191, 255, 150), alpha));
            window.draw(highlight);
        }

        sf::Text t(cat.first, font, 10);
        t.setPosition(currentX + 10.f, y);
        t.setFillColor(isSelected ? applyAlpha(sf::Color::White, alpha) : applyAlpha(sf::Color(130, 130, 140), alpha));
        window.draw(t);
        categoryBounds.push_back({ sf::FloatRect(currentX, y - 4.f, 80.f, 26.f), cat.second });
        y += 32.f;
    }
}

void AssetBrowserPanel::drawAssetGrid(sf::RenderWindow& window, float currentX, sf::Uint8 alpha) {
    auto assets = assetManager.getAssetsByCategory(currentCategory);
    float startX = currentX + 92.f;
    float startY = panelBounds.top + 55.f;

    for (size_t i = 0; i < assets.size(); ++i) {
        float ax = startX + (i % 2) * 85.f;
        float ay = startY + (i / 2) * 105.f;
        bool isSelected = (assets[i]->id == selectedAssetId);

        // Frame Background
        sf::RectangleShape frame(sf::Vector2f(75.f, 75.f));
        frame.setPosition(ax, ay);
        frame.setFillColor(applyAlpha(sf::Color(25, 25, 30, 200), alpha));
        frame.setOutlineThickness(1.f);
        frame.setOutlineColor(isSelected ? applyAlpha(sf::Color(0, 191, 255, 255), alpha) : applyAlpha(sf::Color(255, 255, 255, 10), alpha));
        window.draw(frame);

        // Thumbnail
        sf::RectangleShape thumb(sf::Vector2f(65.f, 65.f));
        thumb.setPosition(ax + 5.f, ay + 5.f);
        if (assets[i]->thumbnailLoaded) {
            thumb.setTexture(&assets[i]->thumbnail);
        }
        else {
            thumb.setFillColor(applyAlpha(sf::Color(40, 40, 45, 255), alpha));
        }
        window.draw(thumb);

        // Name text
        sf::Text nameTxt(assets[i]->filename, font, 9);
        nameTxt.setPosition(ax, ay + 80.f);
        if (nameTxt.getLocalBounds().width > 75.f) {
            std::string trunc = assets[i]->filename.substr(0, 9) + "...";
            nameTxt.setString(trunc);
        }
        nameTxt.setFillColor(isSelected ? applyAlpha(sf::Color(0, 191, 255, 255), alpha) : applyAlpha(sf::Color(170, 170, 180), alpha));
        window.draw(nameTxt);
    }
}

void AssetBrowserPanel::drawProperties(sf::RenderWindow& window, float currentX, sf::Uint8 alpha) {
    sf::RectangleShape propArea(sf::Vector2f(panelBounds.width, 110.f));
    propArea.setPosition(currentX, panelBounds.top + panelBounds.height - 110.f);
    propArea.setFillColor(applyAlpha(sf::Color(22, 22, 26, 255), alpha));
    propArea.setOutlineThickness(1.f);
    propArea.setOutlineColor(applyAlpha(sf::Color(255, 255, 255, 20), alpha));
    window.draw(propArea);

    sf::RectangleShape propHeader(sf::Vector2f(panelBounds.width, 25.f));
    propHeader.setPosition(currentX, panelBounds.top + panelBounds.height - 110.f);
    propHeader.setFillColor(applyAlpha(sf::Color(30, 30, 35, 255), alpha));
    window.draw(propHeader);

    sf::Text headerTxt("PROPERTIES", font, 9);
    headerTxt.setPosition(currentX + 15.f, panelBounds.top + panelBounds.height - 104.f);
    headerTxt.setFillColor(applyAlpha(sf::Color(130, 130, 140), alpha));
    headerTxt.setLetterSpacing(1.5f);
    window.draw(headerTxt);

    AssetRecord* selectedAsset = assetManager.getAsset(selectedAssetId);
    if (selectedAsset) {
        sf::Text propTxt("Name:  " + selectedAsset->filename + "\nType:  " + selectedAsset->extension + "\nSize:  " + std::to_string(selectedAsset->fileSize / 1024) + " KB", font, 10);
        propTxt.setPosition(currentX + 15.f, panelBounds.top + panelBounds.height - 75.f);
        propTxt.setFillColor(applyAlpha(sf::Color(200, 200, 210), alpha));
        propTxt.setLineSpacing(1.6f);
        window.draw(propTxt);
    }
    else {
        sf::Text propTxt("No asset selected.", font, 10);
        propTxt.setPosition(currentX + 15.f, panelBounds.top + panelBounds.height - 75.f);
        propTxt.setFillColor(applyAlpha(sf::Color(100, 100, 110), alpha));
        window.draw(propTxt);
    }
}

void AssetBrowserPanel::triggerImport() {
    std::string file = NativeDialogs::openFileDialog("Image Files\0*.png;*.jpg;*.jpeg;*.bmp;*.webp\0All Files\0*.*\0");
    if (!file.empty()) {
        assetManager.importAssets({ file });
    }
}