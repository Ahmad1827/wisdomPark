#include "ExportModal.h"
#include "../../core/NativeDialogs.h"
#include "../UITheme.h"
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <filesystem>

ExportModal::ExportModal() : isOpen(false), transparentBg(true), autoCrop(false), linkedCanvas(nullptr), activeFrame(0) {}

void ExportModal::init() {
    font.loadFromFile("assets/font.otf");

    overlay.setSize(sf::Vector2f(1920.f, 1080.f));
    overlay.setFillColor(sf::Color(10, 4, 16, 220));

    modalBounds = sf::FloatRect(1920.f / 2.f - 470.f, 1080.f / 2.f - 310.f, 940.f, 620.f);

    closeBtnBounds = sf::FloatRect(modalBounds.left + modalBounds.width - 110.f, modalBounds.top + 20.f, 90.f, 32.f);
    previewAreaBounds = sf::FloatRect(modalBounds.left + 24.f, modalBounds.top + 70.f, 580.f, 520.f);

    transCheckboxBounds = sf::FloatRect(modalBounds.left + 630.f, modalBounds.top + 140.f, 280.f, 36.f);
    cropCheckboxBounds = sf::FloatRect(modalBounds.left + 630.f, modalBounds.top + 186.f, 280.f, 36.f);

    exportPngBtnBounds = sf::FloatRect(modalBounds.left + 630.f, modalBounds.top + 460.f, 280.f, 54.f);
    exportSheetBtnBounds = sf::FloatRect(modalBounds.left + 630.f, modalBounds.top + 526.f, 280.f, 54.f);
}

void ExportModal::open(Canvas& canvas, int frameIndex) {
    linkedCanvas = &canvas;
    activeFrame = frameIndex;
    isOpen = true;
    updatePreview();
}

void ExportModal::close() {
    isOpen = false;
    linkedCanvas = nullptr;
}

bool ExportModal::getIsOpen() const {
    return isOpen;
}

void ExportModal::updatePreview() {
    if (!linkedCanvas) return;

    sf::Image flatImg = ExportManager::flattenFrame(*linkedCanvas, activeFrame);
    sf::IntRect crop = autoCrop ? ExportManager::calculateAutoCrop(flatImg) : sf::IntRect(0, 0, flatImg.getSize().x, flatImg.getSize().y);
    sf::Image finalImg = ExportManager::applyCropAndBackground(flatImg, crop, transparentBg);

    previewTex.loadFromImage(finalImg);
    previewSprite.setTexture(previewTex, true);

    float sX = (previewAreaBounds.width - 24.f) / static_cast<float>(previewTex.getSize().x);
    float sY = (previewAreaBounds.height - 24.f) / static_cast<float>(previewTex.getSize().y);
    float scale = std::min({ sX, sY, 1.0f });

    previewSprite.setScale(scale, scale);

    float px = previewAreaBounds.left + (previewAreaBounds.width - previewTex.getSize().x * scale) / 2.f;
    float py = previewAreaBounds.top + (previewAreaBounds.height - previewTex.getSize().y * scale) / 2.f;
    previewSprite.setPosition(std::floor(px), std::floor(py));

    size_t estimatedBytes = finalImg.getSize().x * finalImg.getSize().y * 4;
    float mb = static_cast<float>(estimatedBytes) / (1024.f * 1024.f);

    std::stringstream ss;
    ss << "CANVAS SPECIFICATIONS\n\n";
    ss << "Resolution : " << finalImg.getSize().x << " x " << finalImg.getSize().y << " px\n";
    ss << "Est Size   : ~" << std::fixed << std::setprecision(2) << mb << " MB\n";
    ss << "Timeline   : " << linkedCanvas->getFrameCount() << " Frames\n";
    ss << "Mode       : " << (linkedCanvas->getPixelMode() ? "Pixel Perfect Grid" : "Dynamic RGBA");
    infoString = ss.str();
}

void ExportModal::updateHover(sf::Vector2f mousePos) {}

void ExportModal::handleEvent(const sf::Event& event, sf::RenderWindow& window) {
    if (!isOpen) return;

    sf::Vector2i pixelPos = sf::Mouse::getPosition(window);
    sf::Vector2f mousePos = window.mapPixelToCoords(pixelPos);

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        if (closeBtnBounds.contains(mousePos)) {
            close();
        }
        else if (transCheckboxBounds.contains(mousePos)) {
            transparentBg = !transparentBg;
            updatePreview();
        }
        else if (cropCheckboxBounds.contains(mousePos)) {
            autoCrop = !autoCrop;
            updatePreview();
        }
        else if (exportPngBtnBounds.contains(mousePos)) {
            if (linkedCanvas->getFrameCount() > 1) {
                std::string folder = NativeDialogs::selectFolderDialog();
                if (!folder.empty()) {
                    ExportManager::exportPNGSequence(*linkedCanvas, folder, transparentBg, autoCrop);
                    close();
                }
            }
            else {
                std::string file = NativeDialogs::saveFileDialog("PNG Files\0*.png\0", "png", "export.png");
                if (!file.empty()) {
                    ExportManager::exportSingleImage(*linkedCanvas, activeFrame, file, transparentBg, autoCrop);
                    close();
                }
            }
        }
        else if (exportSheetBtnBounds.contains(mousePos)) {
            std::string file = NativeDialogs::saveFileDialog("PNG Files\0*.png\0", "png", "spritesheet.png");
            if (!file.empty()) {
                ExportManager::exportSpriteSheet(*linkedCanvas, file, 10, transparentBg, autoCrop);
                close();
            }
        }
    }
}

void ExportModal::draw(sf::RenderWindow& window) {
    if (!isOpen) return;

    window.draw(overlay);

    WisdomUI::Theme::DrawSunsetPanel(window, modalBounds, 1.0f);

    WisdomUI::Theme::DrawCrispText(window, font, ":: MASTER EXPORT STUDIO ::", 14, modalBounds.left + 24.f, modalBounds.top + 26.f, WisdomUI::Theme::SunsetAmber, sf::Color(14, 6, 20));

    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

    WisdomUI::Theme::DrawSunsetButton(window, closeBtnBounds, "Cancel", font, 11, false, closeBtnBounds.contains(mPos), false, 1.0f);

    sf::RectangleShape previewFrame(sf::Vector2f(previewAreaBounds.width, previewAreaBounds.height));
    previewFrame.setPosition(previewAreaBounds.left, previewAreaBounds.top);
    previewFrame.setFillColor(WisdomUI::Theme::SunsetDeepDark);
    previewFrame.setOutlineThickness(1.5f);
    previewFrame.setOutlineColor(WisdomUI::Theme::SunsetPlum);
    window.draw(previewFrame);

    if (transparentBg) {
        float step = 16.f;
        sf::RectangleShape c1(sf::Vector2f(step, step)); c1.setFillColor(sf::Color(32, 16, 44));
        sf::RectangleShape c2(sf::Vector2f(step, step)); c2.setFillColor(sf::Color(44, 22, 58));

        for (float y = previewAreaBounds.top; y < previewAreaBounds.top + previewAreaBounds.height; y += step) {
            for (float x = previewAreaBounds.left; x < previewAreaBounds.left + previewAreaBounds.width; x += step) {
                if (x + step > previewAreaBounds.left + previewAreaBounds.width || y + step > previewAreaBounds.top + previewAreaBounds.height) continue;
                bool alt = (static_cast<int>((x - previewAreaBounds.left) / step) + static_cast<int>((y - previewAreaBounds.top) / step)) % 2 == 0;
                sf::RectangleShape& r = alt ? c1 : c2;
                r.setPosition(x, y);
                window.draw(r);
            }
        }
    }

    window.draw(previewSprite);

    WisdomUI::Theme::DrawSunsetButton(window, transCheckboxBounds, transparentBg ? "[X] Transparent BG" : "[  ] Transparent BG", font, 12, transparentBg, transCheckboxBounds.contains(mPos), transparentBg, 1.0f);
    WisdomUI::Theme::DrawSunsetButton(window, cropCheckboxBounds, autoCrop ? "[X] Auto Bounding Crop" : "[  ] Auto Bounding Crop", font, 12, autoCrop, cropCheckboxBounds.contains(mPos), autoCrop, 1.0f);

    sf::FloatRect infoCard(modalBounds.left + 630.f, modalBounds.top + 240.f, 280.f, 200.f);
    sf::RectangleShape infoBg(sf::Vector2f(infoCard.width, infoCard.height));
    infoBg.setPosition(infoCard.left, infoCard.top);
    infoBg.setFillColor(WisdomUI::Theme::SunsetDeepDark);
    infoBg.setOutlineThickness(1.f);
    infoBg.setOutlineColor(WisdomUI::Theme::SunsetPlum);
    window.draw(infoBg);

    sf::Text iText(infoString, font, 11);
    iText.setPosition(infoCard.left + 16.f, infoCard.top + 16.f);
    iText.setFillColor(WisdomUI::Theme::TextSecondary);
    iText.setLineSpacing(1.4f);
    window.draw(iText);

    std::string pngLabel = (linkedCanvas && linkedCanvas->getFrameCount() > 1) ? "EXPORT PNG SEQUENCE" : "EXPORT SINGLE PNG";
    WisdomUI::Theme::DrawSunsetButton(window, exportPngBtnBounds, pngLabel, font, 12, false, exportPngBtnBounds.contains(mPos), true, 1.0f);
    WisdomUI::Theme::DrawSunsetButton(window, exportSheetBtnBounds, "EXPORT SPRITE SHEET", font, 12, false, exportSheetBtnBounds.contains(mPos), true, 1.0f);
}