#include "AssetBrowserPanel.h"
#include "../core/NativeDialogs.h"
#include "../UI/UITheme.h"
#include <algorithm>
#include <cmath>

AssetBrowserPanel::AssetBrowserPanel(AssetManager& am, const sf::Font& f)
    : assetManager(am), font(f), currentCategory(AssetType::Image), viewMode(BrowserView::Grid),
    selectedAssetId(""), position(1460.f, 78.f), size(390.f, 540.f), isVisible(false), isDraggingAsset(false),
    scrollY(0.0f), maxScrollY(0.0f) {}

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
    if (!isDraggingPanel) {
        position = sf::Vector2f(bounds.left, bounds.top);
    }
    size = sf::Vector2f(bounds.width, bounds.height);
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

    sf::FloatRect panelBounds(position.x, position.y, size.x, size.y);
    WisdomUI::Theme::DrawSunsetPanel(window, panelBounds, 1.0f);

    sf::FloatRect headerGrip(position.x + 8.f, position.y + 6.f, size.x - 16.f, 26.f);
    sf::RectangleShape gripBg(sf::Vector2f(headerGrip.width, headerGrip.height));
    gripBg.setPosition(headerGrip.left, headerGrip.top);
    gripBg.setFillColor(WisdomUI::Theme::SunsetDeepDark);
    gripBg.setOutlineThickness(1.f);
    gripBg.setOutlineColor(WisdomUI::Theme::SunsetPlum);
    window.draw(gripBg);

    WisdomUI::Theme::DrawCrispText(window, font, ":: ASSET VAULT ::", 12, headerGrip.left + headerGrip.width / 2.0f, headerGrip.top + headerGrip.height / 2.0f, WisdomUI::Theme::SunsetAmber, sf::Color(14, 6, 20), true, true);

    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

    importBtnBounds = sf::FloatRect(position.x + size.x - 88.f, position.y + 36.f, 76.f, 24.f);
    WisdomUI::Theme::DrawSunsetButton(window, importBtnBounds, "Import", font, 11, false, importBtnBounds.contains(mPos), true, 1.0f);

    categoryBounds.clear();
    std::vector<std::pair<std::string, AssetType>> cats = {
        {"Images", AssetType::Image},
        {"Audio", AssetType::Audio},
        {"Fonts", AssetType::Font},
        {"Brushes", AssetType::Brush},
        {"Patterns", AssetType::Pattern},
        {"AI Assets", AssetType::AI}
    };

    float catY = position.y + 66.f;
    for (const auto& cat : cats) {
        bool isSelected = (currentCategory == cat.second);
        sf::FloatRect cBounds(position.x + 10.f, catY, 82.f, 24.f);
        categoryBounds.push_back({ cBounds, cat.second });
        WisdomUI::Theme::DrawSunsetButton(window, cBounds, cat.first, font, 10, isSelected, cBounds.contains(mPos), isSelected, 1.0f);
        catY += 28.f;
    }

    gridAreaBounds = sf::FloatRect(position.x + 98.f, position.y + 66.f, size.x - 108.f, size.y - 146.f);

    auto assets = assetManager.getAssetsByCategory(currentCategory);
    float startX = gridAreaBounds.left;
    float startY = gridAreaBounds.top - scrollY;

    deleteBtnBounds.clear();

    float totalGridHeight = std::ceil(static_cast<float>(assets.size()) / 2.0f) * 105.0f;
    maxScrollY = std::max(0.0f, totalGridHeight - gridAreaBounds.height);
    scrollY = std::clamp(scrollY, 0.0f, maxScrollY);

    for (size_t i = 0; i < assets.size(); ++i) {
        float ax = startX + (i % 2) * 136.f;
        float ay = startY + (i / 2) * 105.f;

        if (ay + 95.f < gridAreaBounds.top || ay > gridAreaBounds.top + gridAreaBounds.height) {
            continue;
        }

        bool isSelected = (assets[i]->id == selectedAssetId);

        sf::FloatRect cardRect(ax, ay, 130.f, 95.f);
        sf::RectangleShape frame(sf::Vector2f(cardRect.width, cardRect.height));
        frame.setPosition(cardRect.left, cardRect.top);
        frame.setFillColor(isSelected ? WisdomUI::Theme::SunsetSkyMid : WisdomUI::Theme::SunsetDeepDark);
        frame.setOutlineThickness(1.5f);
        frame.setOutlineColor(isSelected ? WisdomUI::Theme::SunsetAmber : WisdomUI::Theme::SunsetPlum);
        window.draw(frame);

        sf::RectangleShape thumb(sf::Vector2f(120.f, 65.f));
        thumb.setPosition(ax + 5.f, ay + 5.f);
        if (assets[i]->thumbnailLoaded) {
            thumb.setTexture(&assets[i]->thumbnail);
        }
        else {
            thumb.setFillColor(WisdomUI::Theme::SunsetSkyTop);
        }
        window.draw(thumb);

        sf::FloatRect delRect(ax + cardRect.width - 20.f, ay + 4.f, 16.f, 16.f);
        deleteBtnBounds.push_back({ delRect, assets[i]->id });

        sf::RectangleShape delBg(sf::Vector2f(delRect.width, delRect.height));
        delBg.setPosition(delRect.left, delRect.top);
        delBg.setFillColor(delRect.contains(mPos) ? sf::Color(220, 40, 60, 240) : sf::Color(14, 6, 20, 200));
        delBg.setOutlineThickness(1.f);
        delBg.setOutlineColor(WisdomUI::Theme::SunsetPlum);
        window.draw(delBg);

        WisdomUI::Theme::DrawCrispText(window, font, "x", 11, delRect.left + delRect.width / 2.0f, delRect.top + delRect.height / 2.0f - 1.f, sf::Color::White, sf::Color::Transparent, true, true);

        std::string nameStr = assets[i]->filename;
        if (nameStr.length() > 16) nameStr = nameStr.substr(0, 14) + "..";
        WisdomUI::Theme::DrawCrispText(window, font, nameStr, 10, ax + 65.f, ay + 80.f, isSelected ? WisdomUI::Theme::SunsetGold : WisdomUI::Theme::TextSecondary, sf::Color::Transparent, true, true);
    }

    if (maxScrollY > 0.0f) {
        float scrollTrackH = gridAreaBounds.height;
        float thumbH = std::max(20.0f, (gridAreaBounds.height / totalGridHeight) * scrollTrackH);
        float thumbY = gridAreaBounds.top + (scrollY / maxScrollY) * (scrollTrackH - thumbH);

        sf::RectangleShape scrollTrack(sf::Vector2f(4.f, scrollTrackH));
        scrollTrack.setPosition(gridAreaBounds.left + gridAreaBounds.width - 6.f, gridAreaBounds.top);
        scrollTrack.setFillColor(WisdomUI::Theme::SunsetDeepDark);
        window.draw(scrollTrack);

        sf::RectangleShape scrollThumb(sf::Vector2f(4.f, thumbH));
        scrollThumb.setPosition(gridAreaBounds.left + gridAreaBounds.width - 6.f, thumbY);
        scrollThumb.setFillColor(WisdomUI::Theme::SunsetGold);
        window.draw(scrollThumb);
    }

    sf::FloatRect propArea(position.x + 10.f, position.y + size.y - 75.f, size.x - 20.f, 65.f);
    sf::RectangleShape pBox(sf::Vector2f(propArea.width, propArea.height));
    pBox.setPosition(propArea.left, propArea.top);
    pBox.setFillColor(WisdomUI::Theme::SunsetDeepDark);
    pBox.setOutlineThickness(1.f);
    pBox.setOutlineColor(WisdomUI::Theme::SunsetPlum);
    window.draw(pBox);

    AssetRecord* selectedAsset = assetManager.getAsset(selectedAssetId);
    if (selectedAsset) {
        std::string info = selectedAsset->filename + " (" + std::to_string(selectedAsset->fileSize / 1024) + " KB)";
        WisdomUI::Theme::DrawCrispText(window, font, info, 11, propArea.left + 8.f, propArea.top + 8.f, WisdomUI::Theme::SunsetAmber);
        WisdomUI::Theme::DrawCrispText(window, font, "Drag asset directly onto canvas to place", 10, propArea.left + 8.f, propArea.top + 28.f, WisdomUI::Theme::TextSecondary);
    }
    else {
        WisdomUI::Theme::DrawCrispText(window, font, "No asset selected", 11, propArea.left + 8.f, propArea.top + 24.f, WisdomUI::Theme::SunsetPlum);
    }
}

void AssetBrowserPanel::handleEvent(const sf::Event& event, const sf::RenderWindow& window, Canvas& canvas, int currentFrame) {
    if (!isVisible) return;

    sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    sf::FloatRect headerGrip(position.x, position.y, size.x, 34.f);

    if (event.type == sf::Event::MouseWheelScrolled) {
        if (gridAreaBounds.contains(mousePos)) {
            scrollY = std::clamp(scrollY - event.mouseWheelScroll.delta * 35.0f, 0.0f, maxScrollY);
            return;
        }
    }

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        for (const auto& delItem : deleteBtnBounds) {
            if (delItem.first.contains(mousePos)) {
                if (selectedAssetId == delItem.second) {
                    selectedAssetId = "";
                }
                assetManager.removeAsset(delItem.second);
                return;
            }
        }

        if (headerGrip.contains(mousePos)) {
            isDraggingPanel = true;
            dragOffset = mousePos - position;
            return;
        }

        if (importBtnBounds.contains(mousePos)) {
            triggerImport();
            return;
        }

        for (const auto& cb : categoryBounds) {
            if (cb.first.contains(mousePos)) {
                currentCategory = cb.second;
                selectedAssetId = "";
                scrollY = 0.0f;
                return;
            }
        }

        auto assets = assetManager.getAssetsByCategory(currentCategory);
        float startX = gridAreaBounds.left;
        float startY = gridAreaBounds.top - scrollY;

        for (size_t i = 0; i < assets.size(); ++i) {
            float ax = startX + (i % 2) * 136.f;
            float ay = startY + (i / 2) * 105.f;

            if (ay + 95.f < gridAreaBounds.top || ay > gridAreaBounds.top + gridAreaBounds.height) {
                continue;
            }

            if (sf::FloatRect(ax, ay, 130.f, 95.f).contains(mousePos)) {
                selectedAssetId = assets[i]->id;
                isDraggingAsset = true;
                dragStart = mousePos;
                return;
            }
        }
    }
    else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        isDraggingPanel = false;
        if (isDraggingAsset) {
            isDraggingAsset = false;
            if (!sf::FloatRect(position.x, position.y, size.x, size.y).contains(mousePos)) {
                handleDragAndDrop(mousePos, window, canvas, currentFrame);
            }
        }
    }
    else if (event.type == sf::Event::MouseMoved && isDraggingPanel) {
        position = mousePos - dragOffset;
        position.x = std::clamp(position.x, 56.f, 1920.f - size.x);
        position.y = std::clamp(position.y, 40.f, 1080.f - size.y);
    }
}

void AssetBrowserPanel::handleDragAndDrop(const sf::Vector2f& dropPos, const sf::RenderWindow& window, Canvas& canvas, int currentFrame) {
    AssetRecord* selectedAsset = assetManager.getAsset(selectedAssetId);
    if (!selectedAsset) return;

    if (selectedAsset->type == AssetType::Image) {
        canvas.importImageToActiveLayer(selectedAsset->filepath, currentFrame);
    }
}

void AssetBrowserPanel::triggerImport() {
    std::string file = NativeDialogs::openFileDialog("Image Files\0*.png;*.jpg;*.jpeg;*.jfif;*.bmp;*.webp\0All Files\0*.*\0");
    if (!file.empty()) {
        assetManager.importAssets({ file });
    }
}