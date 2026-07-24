#include "ExportModal.h"
#include "../../core/NativeDialogs.h"
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <filesystem>

ExportModal::ExportModal() : isOpen(false), transparentBg(true), autoCrop(false), linkedCanvas(nullptr), activeFrame(0) {}

void ExportModal::init() {
    font.loadFromFile("assets/font.otf");

    overlay.setSize(sf::Vector2f(1920.f, 1080.f));
    overlay.setFillColor(sf::Color(0, 0, 0, 200));

    modalBg.setSize(sf::Vector2f(900.f, 600.f));
    modalBg.setPosition(1920.f / 2.f - 450.f, 1080.f / 2.f - 300.f);
    modalBg.setFillColor(sf::Color(25, 25, 30, 255));
    modalBg.setOutlineThickness(2.f);
    modalBg.setOutlineColor(sf::Color(100, 100, 110, 100));

    title.setFont(font);
    title.setString("Export Options");
    title.setCharacterSize(24);
    title.setFillColor(sf::Color::White);
    title.setPosition(modalBg.getPosition().x + 30.f, modalBg.getPosition().y + 30.f);

    closeBtn.setSize(sf::Vector2f(100.f, 40.f));
    closeBtn.setPosition(modalBg.getPosition().x + 770.f, modalBg.getPosition().y + 30.f);
    closeBtn.setFillColor(sf::Color(50, 50, 60));

    closeText.setFont(font);
    closeText.setString("Cancel");
    closeText.setCharacterSize(16);
    closeText.setFillColor(sf::Color::White);
    closeText.setPosition(closeBtn.getPosition().x + 25.f, closeBtn.getPosition().y + 10.f);

    exportPngBtn.setSize(sf::Vector2f(180.f, 50.f));
    exportPngBtn.setPosition(modalBg.getPosition().x + 670.f, modalBg.getPosition().y + 450.f);
    exportPngBtn.setFillColor(sf::Color(0, 122, 204));

    exportPngText.setFont(font);
    exportPngText.setString("Export PNG");
    exportPngText.setCharacterSize(18);
    exportPngText.setFillColor(sf::Color::White);
    exportPngText.setPosition(exportPngBtn.getPosition().x + 40.f, exportPngBtn.getPosition().y + 13.f);

    exportSheetBtn.setSize(sf::Vector2f(180.f, 50.f));
    exportSheetBtn.setPosition(modalBg.getPosition().x + 670.f, modalBg.getPosition().y + 520.f);
    exportSheetBtn.setFillColor(sf::Color(0, 150, 50));

    exportSheetText.setFont(font);
    exportSheetText.setString("Sprite Sheet");
    exportSheetText.setCharacterSize(18);
    exportSheetText.setFillColor(sf::Color::White);
    exportSheetText.setPosition(exportSheetBtn.getPosition().x + 40.f, exportSheetBtn.getPosition().y + 13.f);

    transCheckbox.setSize(sf::Vector2f(24.f, 24.f));
    transCheckbox.setPosition(modalBg.getPosition().x + 670.f, modalBg.getPosition().y + 150.f);
    transCheckbox.setFillColor(sf::Color(50, 50, 60));
    transCheckbox.setOutlineThickness(1.f);

    transText.setFont(font);
    transText.setString("Transparent BG");
    transText.setCharacterSize(14);
    transText.setFillColor(sf::Color::White);
    transText.setPosition(modalBg.getPosition().x + 705.f, modalBg.getPosition().y + 153.f);

    cropCheckbox.setSize(sf::Vector2f(24.f, 24.f));
    cropCheckbox.setPosition(modalBg.getPosition().x + 670.f, modalBg.getPosition().y + 200.f);
    cropCheckbox.setFillColor(sf::Color(50, 50, 60));
    cropCheckbox.setOutlineThickness(1.f);

    cropText.setFont(font);
    cropText.setString("Auto Crop");
    cropText.setCharacterSize(14);
    cropText.setFillColor(sf::Color::White);
    cropText.setPosition(modalBg.getPosition().x + 705.f, modalBg.getPosition().y + 203.f);

    previewArea.setSize(sf::Vector2f(600.f, 450.f));
    previewArea.setPosition(modalBg.getPosition().x + 30.f, modalBg.getPosition().y + 100.f);
    previewArea.setFillColor(sf::Color(15, 15, 18));
    previewArea.setOutlineThickness(1.f);
    previewArea.setOutlineColor(sf::Color(100, 100, 110));

    infoText.setFont(font);
    infoText.setCharacterSize(14);
    infoText.setFillColor(sf::Color(180, 180, 180));
    infoText.setPosition(modalBg.getPosition().x + 670.f, modalBg.getPosition().y + 260.f);
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

    float sX = 580.f / static_cast<float>(previewTex.getSize().x);
    float sY = 430.f / static_cast<float>(previewTex.getSize().y);
    float scale = std::min(std::min(sX, sY), 1.0f);

    previewSprite.setScale(scale, scale);

    float px = previewArea.getPosition().x + (previewArea.getSize().x - previewTex.getSize().x * scale) / 2.f;
    float py = previewArea.getPosition().y + (previewArea.getSize().y - previewTex.getSize().y * scale) / 2.f;
    previewSprite.setPosition(px, py);

    size_t estimatedBytes = finalImg.getSize().x * finalImg.getSize().y * 4;
    float mb = estimatedBytes / (1024.f * 1024.f);

    std::stringstream ss;
    ss << "Resolution:\n" << finalImg.getSize().x << " x " << finalImg.getSize().y << " px\n\n";
    ss << "Est Size (Single):\n~" << std::fixed << std::setprecision(2) << mb << " MB\n\n";
    ss << "Total Frames:\n" << linkedCanvas->getFrameCount();
    infoText.setString(ss.str());

    transCheckbox.setFillColor(transparentBg ? sf::Color(0, 191, 255) : sf::Color(50, 50, 60));
    cropCheckbox.setFillColor(autoCrop ? sf::Color(0, 191, 255) : sf::Color(50, 50, 60));
}

void ExportModal::updateHover(sf::Vector2f mousePos) {
    if (!isOpen) return;
    closeBtn.setFillColor(closeBtn.getGlobalBounds().contains(mousePos) ? sf::Color(80, 80, 90) : sf::Color(50, 50, 60));
    exportPngBtn.setFillColor(exportPngBtn.getGlobalBounds().contains(mousePos) ? sf::Color(0, 150, 255) : sf::Color(0, 122, 204));
    exportSheetBtn.setFillColor(exportSheetBtn.getGlobalBounds().contains(mousePos) ? sf::Color(0, 180, 80) : sf::Color(0, 150, 50));
    transCheckbox.setOutlineColor(transCheckbox.getGlobalBounds().contains(mousePos) ? sf::Color::White : sf::Color::Transparent);
    cropCheckbox.setOutlineColor(cropCheckbox.getGlobalBounds().contains(mousePos) ? sf::Color::White : sf::Color::Transparent);
}

void ExportModal::handleEvent(const sf::Event& event, sf::RenderWindow& window) {
    if (!isOpen) return;

    // FIX: Map mouse pixels to windowed coords
    sf::Vector2i pixelPos = sf::Mouse::getPosition(window);
    sf::Vector2f mousePos = window.mapPixelToCoords(pixelPos);

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        if (closeBtn.getGlobalBounds().contains(mousePos)) {
            close();
        }
        else if (transCheckbox.getGlobalBounds().contains(mousePos)) {
            transparentBg = !transparentBg;
            updatePreview();
        }
        else if (cropCheckbox.getGlobalBounds().contains(mousePos)) {
            autoCrop = !autoCrop;
            updatePreview();
        }
        else if (exportPngBtn.getGlobalBounds().contains(mousePos)) {
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
        else if (exportSheetBtn.getGlobalBounds().contains(mousePos)) {
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
    window.draw(modalBg);
    window.draw(title);
    window.draw(closeBtn);
    window.draw(closeText);

    window.draw(exportPngBtn);
    window.draw(exportPngText);
    window.draw(exportSheetBtn);
    window.draw(exportSheetText);

    window.draw(transCheckbox);
    window.draw(transText);
    window.draw(cropCheckbox);
    window.draw(cropText);

    window.draw(previewArea);

    if (transparentBg) {
        sf::RectangleShape check1(sf::Vector2f(20.f, 20.f)); check1.setFillColor(sf::Color(100, 100, 100));
        sf::RectangleShape check2(sf::Vector2f(20.f, 20.f)); check2.setFillColor(sf::Color(150, 150, 150));
        for (float y = previewArea.getPosition().y; y < previewArea.getPosition().y + previewArea.getSize().y; y += 20.f) {
            for (float x = previewArea.getPosition().x; x < previewArea.getPosition().x + previewArea.getSize().x; x += 20.f) {
                if (x + 20.f > previewArea.getPosition().x + previewArea.getSize().x || y + 20.f > previewArea.getPosition().y + previewArea.getSize().y) continue;
                bool alt = (static_cast<int>((x - previewArea.getPosition().x) / 20.f) + static_cast<int>((y - previewArea.getPosition().y) / 20.f)) % 2 == 0;
                sf::RectangleShape& r = alt ? check1 : check2;
                r.setPosition(x, y);
                window.draw(r);
            }
        }
    }

    window.draw(previewSprite);
    window.draw(infoText);
}