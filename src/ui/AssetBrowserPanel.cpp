#include "AssetBrowserPanel.h"
#include "../core/NativeDialogs.h"
#include "../UI/UITheme.h"
#include <algorithm>
#include <cmath>

AssetBrowserPanel::AssetBrowserPanel(AssetManager& am, const sf::Font& f)
    : assetManager(am), font(f), currentCategory(AssetType::Image), viewMode(BrowserView::Grid),
    selectedAssetId(""), position(1400.f, 75.f), size(440.f, 600.f), isVisible(false), isDraggingAsset(false),
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
    size = sf::Vector2f(std::max(430.f, bounds.width), std::max(580.f, bounds.height));
}

void AssetBrowserPanel::update(float dt) {
    if (!isVisible) return;

    AssetRecord* selectedAsset = assetManager.getAsset(selectedAssetId);
    if (selectedAsset && !selectedAsset->thumbnailLoaded) {
        assetManager.requestThumbnail(selectedAsset);
    }
}

void AssetBrowserPanel::drawTooltip(sf::RenderWindow& window, const std::string& text, sf::Vector2f pos) {
    sf::Text tipText(text, font, 13);
    sf::FloatRect bounds = tipText.getLocalBounds();

    float padX = 10.f;
    float padY = 6.f;
    float w = bounds.width + padX * 2.f;
    float h = bounds.height + padY * 2.f + 4.f;

    float x = pos.x - w - 12.f;
    float y = pos.y + 14.f;

    if (x < 10.f) x = pos.x + 16.f;
    if (y + h > 1070.f) y = pos.y - h - 6.f;

    sf::RectangleShape bg(sf::Vector2f(w, h));
    bg.setPosition(x, y);
    bg.setFillColor(sf::Color(18, 12, 24, 250));
    bg.setOutlineThickness(1.5f);
    bg.setOutlineColor(WisdomUI::Theme::Gold);

    tipText.setPosition(x + padX, y + padY - 2.f);
    tipText.setFillColor(sf::Color::White);

    window.draw(bg);
    window.draw(tipText);
}

void AssetBrowserPanel::draw(sf::RenderWindow& window) {
    if (!isVisible) return;

    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    std::string hoveredTooltip = "";

    sf::FloatRect panelBounds(position.x, position.y, size.x, size.y);
    WisdomUI::Theme::DrawSunsetPanel(window, panelBounds, 1.0f);

    sf::FloatRect headerGrip(position.x + 8.f, position.y + 6.f, size.x - 16.f, 32.f);
    sf::RectangleShape gripBg(sf::Vector2f(headerGrip.width, headerGrip.height));
    gripBg.setPosition(headerGrip.left, headerGrip.top);
    gripBg.setFillColor(WisdomUI::Theme::SunsetDeepDark);
    gripBg.setOutlineThickness(1.f);
    gripBg.setOutlineColor(WisdomUI::Theme::SunsetPlum);
    window.draw(gripBg);

    WisdomUI::Theme::DrawCrispText(window, font, ":: ASSET VAULT ::", 16, headerGrip.left + headerGrip.width / 2.0f, headerGrip.top + headerGrip.height / 2.0f, WisdomUI::Theme::SunsetAmber, sf::Color(14, 6, 20), true, true);

    importBtnBounds = sf::FloatRect(position.x + size.x - 104.f, position.y + 42.f, 92.f, 28.f);
    bool hovImport = importBtnBounds.contains(mPos);
    if (hovImport) hoveredTooltip = "Import external image or audio files";
    WisdomUI::Theme::DrawSunsetButton(window, importBtnBounds, "Import", font, 13, false, hovImport, true, 1.0f);

    categoryBounds.clear();
    std::vector<std::pair<std::string, AssetType>> cats = {
        {"Images", AssetType::Image},
        {"Audio", AssetType::Audio},
        {"Fonts", AssetType::Font},
        {"Brushes", AssetType::Brush},
        {"Patterns", AssetType::Pattern},
        {"AI Assets", AssetType::AI}
    };

    float catY = position.y + 76.f;
    for (const auto& cat : cats) {
        bool isSelected = (currentCategory == cat.second);
        sf::FloatRect cBounds(position.x + 10.f, catY, 94.f, 28.f);
        categoryBounds.push_back({ cBounds, cat.second });
        bool hovCat = cBounds.contains(mPos);
        if (hovCat) hoveredTooltip = "View " + cat.first + " folder";
        WisdomUI::Theme::DrawSunsetButton(window, cBounds, cat.first, font, 13, isSelected, hovCat, isSelected, 1.0f);
        catY += 34.f;
    }

    gridAreaBounds = sf::FloatRect(position.x + 112.f, position.y + 76.f, size.x - 124.f, size.y - 164.f);

    auto assets = assetManager.getAssetsByCategory(currentCategory);

    float scrollGutter = 12.f;
    float availableW = gridAreaBounds.width - scrollGutter;
    float colGap = 8.f;
    float cardW = std::floor((availableW - colGap) / 2.0f);
    float cardH = 98.f;
    float rowGap = 8.f;
    float rowStep = cardH + rowGap;

    float totalGridHeight = std::ceil(static_cast<float>(assets.size()) / 2.0f) * rowStep;
    maxScrollY = std::max(0.0f, totalGridHeight - gridAreaBounds.height);
    scrollY = std::clamp(scrollY, 0.0f, maxScrollY);

    deleteBtnBounds.clear();

    sf::View savedView = window.getView();
    sf::FloatRect letterboxVp = savedView.getViewport();

    float normX = gridAreaBounds.left / 1920.f;
    float normY = gridAreaBounds.top / 1080.f;
    float normW = gridAreaBounds.width / 1920.f;
    float normH = gridAreaBounds.height / 1080.f;

    sf::FloatRect subVp(
        letterboxVp.left + normX * letterboxVp.width,
        letterboxVp.top + normY * letterboxVp.height,
        normW * letterboxVp.width,
        normH * letterboxVp.height
    );

    sf::View gridSubView(sf::FloatRect(gridAreaBounds.left, gridAreaBounds.top, gridAreaBounds.width, gridAreaBounds.height));
    gridSubView.setViewport(subVp);
    window.setView(gridSubView);

    float startX = gridAreaBounds.left;
    float startY = gridAreaBounds.top - scrollY;

    for (size_t i = 0; i < assets.size(); ++i) {
        float ax = startX + (i % 2) * (cardW + colGap);
        float ay = startY + (i / 2) * rowStep;

        if (ay + cardH < gridAreaBounds.top || ay > gridAreaBounds.top + gridAreaBounds.height) {
            continue;
        }

        bool isSelected = (assets[i]->id == selectedAssetId);

        sf::FloatRect cardRect(ax, ay, cardW, cardH);
        bool hovCard = cardRect.contains(mPos) && gridAreaBounds.contains(mPos);
        if (hovCard) hoveredTooltip = "Click to select, drag onto canvas to place";

        sf::RectangleShape frame(sf::Vector2f(cardRect.width, cardRect.height));
        frame.setPosition(cardRect.left, cardRect.top);
        frame.setFillColor(isSelected ? WisdomUI::Theme::SunsetSkyMid : (hovCard ? sf::Color(35, 25, 45) : WisdomUI::Theme::SunsetDeepDark));
        frame.setOutlineThickness(1.5f);
        frame.setOutlineColor(isSelected ? WisdomUI::Theme::SunsetAmber : (hovCard ? WisdomUI::Theme::SunsetCoral : WisdomUI::Theme::SunsetPlum));
        window.draw(frame);

        sf::RectangleShape thumb(sf::Vector2f(cardW - 8.f, cardH - 32.f));
        thumb.setPosition(ax + 4.f, ay + 4.f);
        if (assets[i]->thumbnailLoaded) {
            thumb.setTexture(&assets[i]->thumbnail);
        }
        else {
            thumb.setFillColor(WisdomUI::Theme::SunsetSkyTop);
        }
        window.draw(thumb);

        sf::FloatRect delRect(ax + cardW - 22.f, ay + 4.f, 18.f, 18.f);
        if (gridAreaBounds.contains(mPos) && delRect.contains(mPos)) {
            hoveredTooltip = "Delete " + assets[i]->filename;
        }

        if (delRect.top + delRect.height >= gridAreaBounds.top && delRect.top <= gridAreaBounds.top + gridAreaBounds.height) {
            deleteBtnBounds.push_back({ delRect, assets[i]->id });
        }

        sf::RectangleShape delBg(sf::Vector2f(delRect.width, delRect.height));
        delBg.setPosition(delRect.left, delRect.top);
        delBg.setFillColor(gridAreaBounds.contains(mPos) && delRect.contains(mPos) ? sf::Color(220, 40, 60, 240) : sf::Color(14, 6, 20, 200));
        delBg.setOutlineThickness(1.f);
        delBg.setOutlineColor(WisdomUI::Theme::SunsetPlum);
        window.draw(delBg);

        WisdomUI::Theme::DrawCrispText(window, font, "x", 12, delRect.left + delRect.width / 2.0f, delRect.top + delRect.height / 2.0f - 1.f, sf::Color::White, sf::Color::Transparent, true, true);

        std::string nameStr = assets[i]->filename;
        size_t maxChars = static_cast<size_t>(std::max(6, static_cast<int>(cardW / 9.5f)));
        if (nameStr.length() > maxChars) nameStr = nameStr.substr(0, maxChars - 2) + "..";
        WisdomUI::Theme::DrawCrispText(window, font, nameStr, 12, ax + cardW / 2.f, ay + cardH - 14.f, isSelected ? WisdomUI::Theme::SunsetGold : WisdomUI::Theme::TextSecondary, sf::Color::Transparent, true, true);
    }

    window.setView(savedView);

    if (maxScrollY > 0.0f) {
        float scrollTrackH = gridAreaBounds.height;
        float thumbH = std::max(24.0f, (gridAreaBounds.height / totalGridHeight) * scrollTrackH);
        float thumbY = gridAreaBounds.top + (scrollY / maxScrollY) * (scrollTrackH - thumbH);
        float trackX = gridAreaBounds.left + gridAreaBounds.width - 5.f;

        sf::RectangleShape scrollTrack(sf::Vector2f(4.f, scrollTrackH));
        scrollTrack.setPosition(trackX, gridAreaBounds.top);
        scrollTrack.setFillColor(WisdomUI::Theme::SunsetDeepDark);
        window.draw(scrollTrack);

        sf::RectangleShape scrollThumb(sf::Vector2f(4.f, thumbH));
        scrollThumb.setPosition(trackX, thumbY);
        scrollThumb.setFillColor(WisdomUI::Theme::SunsetGold);
        window.draw(scrollThumb);
    }

    sf::FloatRect propArea(position.x + 10.f, position.y + size.y - 80.f, size.x - 20.f, 70.f);
    sf::RectangleShape pBox(sf::Vector2f(propArea.width, propArea.height));
    pBox.setPosition(propArea.left, propArea.top);
    pBox.setFillColor(WisdomUI::Theme::SunsetDeepDark);
    pBox.setOutlineThickness(1.f);
    pBox.setOutlineColor(WisdomUI::Theme::SunsetPlum);
    window.draw(pBox);

    AssetRecord* selectedAsset = assetManager.getAsset(selectedAssetId);
    if (selectedAsset) {
        std::string info = selectedAsset->filename + " (" + std::to_string(selectedAsset->fileSize / 1024) + " KB)";
        WisdomUI::Theme::DrawCrispText(window, font, info, 13, propArea.left + 12.f, propArea.top + 12.f, WisdomUI::Theme::SunsetAmber);
        WisdomUI::Theme::DrawCrispText(window, font, "Drag asset directly onto canvas to place", 12, propArea.left + 12.f, propArea.top + 36.f, WisdomUI::Theme::TextSecondary);
    }
    else {
        WisdomUI::Theme::DrawCrispText(window, font, "No asset selected", 13, propArea.left + 12.f, propArea.top + 26.f, WisdomUI::Theme::SunsetPlum);
    }

    if (!hoveredTooltip.empty() && !isDraggingAsset) {
        drawTooltip(window, hoveredTooltip, mPos);
    }
}

bool AssetBrowserPanel::handleEvent(const sf::Event& event, const sf::RenderWindow& window, Canvas& canvas, int currentFrame) {
    if (!isVisible) return false;

    sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    sf::FloatRect panelBounds(position.x, position.y, size.x, size.y);
    sf::FloatRect headerGrip(position.x, position.y, size.x, 34.f);

    if (event.type == sf::Event::MouseWheelScrolled) {
        if (panelBounds.contains(mousePos)) {
            if (gridAreaBounds.contains(mousePos)) {
                scrollY = std::clamp(scrollY - event.mouseWheelScroll.delta * 35.0f, 0.0f, maxScrollY);
            }
            return true;
        }
        return false;
    }

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        if (gridAreaBounds.contains(mousePos)) {
            for (const auto& delItem : deleteBtnBounds) {
                if (delItem.first.contains(mousePos)) {
                    if (selectedAssetId == delItem.second) {
                        selectedAssetId = "";
                    }
                    assetManager.removeAsset(delItem.second);
                    return true;
                }
            }

            auto assets = assetManager.getAssetsByCategory(currentCategory);
            float scrollGutter = 12.f;
            float availableW = gridAreaBounds.width - scrollGutter;
            float colGap = 8.f;
            float cardW = std::floor((availableW - colGap) / 2.0f);
            float cardH = 98.f;
            float rowStep = cardH + 8.f;

            float startX = gridAreaBounds.left;
            float startY = gridAreaBounds.top - scrollY;

            for (size_t i = 0; i < assets.size(); ++i) {
                float ax = startX + (i % 2) * (cardW + colGap);
                float ay = startY + (i / 2) * rowStep;

                if (sf::FloatRect(ax, ay, cardW, cardH).contains(mousePos)) {
                    selectedAssetId = assets[i]->id;
                    isDraggingAsset = true;
                    dragStart = mousePos;
                    return true;
                }
            }
            return true;
        }

        if (headerGrip.contains(mousePos)) {
            isDraggingPanel = true;
            dragOffset = mousePos - position;
            return true;
        }

        if (importBtnBounds.contains(mousePos)) {
            triggerImport();
            return true;
        }

        for (const auto& cb : categoryBounds) {
            if (cb.first.contains(mousePos)) {
                currentCategory = cb.second;
                selectedAssetId = "";
                scrollY = 0.0f;
                return true;
            }
        }

        if (panelBounds.contains(mousePos)) return true;
    }
    else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        if (isDraggingPanel) {
            isDraggingPanel = false;
            return true;
        }
        if (isDraggingAsset) {
            isDraggingAsset = false;
            if (!panelBounds.contains(mousePos)) {
                handleDragAndDrop(mousePos, window, canvas, currentFrame);
            }
            return true;
        }
        if (panelBounds.contains(mousePos)) return true;
    }
    else if (event.type == sf::Event::MouseMoved) {
        if (isDraggingPanel) {
            position = mousePos - dragOffset;
            position.x = std::clamp(position.x, 56.f, 1920.f - size.x);
            position.y = std::clamp(position.y, 40.f, 1080.f - size.y);
            return true;
        }
        if (isDraggingAsset) return true;
        if (panelBounds.contains(mousePos)) return true;
    }

    return false;
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