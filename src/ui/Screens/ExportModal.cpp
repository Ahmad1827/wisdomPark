#include "ExportModal.h"
#include <sstream>
#include <algorithm>
#include <iomanip>

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
    title.setString("Export Preview");
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

    exportBtn.setSize(sf::Vector2f(200.f, 50.f));
    exportBtn.setPosition(modalBg.getPosition().x + 670.f, modalBg.getPosition().y + 520.f);
    exportBtn.setFillColor(sf::Color(0, 122, 204));

    exportText.setFont(font);
    exportText.setString("Save As PNG");
    exportText.setCharacterSize(18);
    exportText.setFillColor(sf::Color::White);
    exportText.setPosition(exportBtn.getPosition().x + 40.f, exportBtn.getPosition().y + 13.f);

    transCheckbox.setSize(sf::Vector2f(24.f, 24.f));
    transCheckbox.setPosition(modalBg.getPosition().x + 670.f, modalBg.getPosition().y + 150.f);
    transCheckbox.setFillColor(sf::Color(50, 50, 60));
    transCheckbox.setOutlineThickness(1.f);

    transText.setFont(font);
    transText.setString("Transparent Background");
    transText.setCharacterSize(14);
    transText.setFillColor(sf::Color::White);
    transText.setPosition(modalBg.getPosition().x + 705.f, modalBg.getPosition().y + 153.f);

    cropCheckbox.setSize(sf::Vector2f(24.f, 24.f));
    cropCheckbox.setPosition(modalBg.getPosition().x + 670.f, modalBg.getPosition().y + 200.f);
    cropCheckbox.setFillColor(sf::Color(50, 50, 60));
    cropCheckbox.setOutlineThickness(1.f);

    cropText.setFont(font);
    cropText.setString("Auto Crop Empty Space");
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
    ss << "Uncompressed Size:\n~" << std::fixed << std::setprecision(2) << mb << " MB\n\n";
    ss << "Total Frames:\n" << linkedCanvas->getFrameCount();
    infoText.setString(ss.str());

    transCheckbox.setFillColor(transparentBg ? sf::Color(0, 191, 255) : sf::Color(50, 50, 60));
    cropCheckbox.setFillColor(autoCrop ? sf::Color(0, 191, 255) : sf::Color(50, 50, 60));
}

void ExportModal::updateHover(sf::Vector2f mousePos) {
    if (!isOpen) return;
    closeBtn.setFillColor(closeBtn.getGlobalBounds().contains(mousePos) ? sf::Color(80, 80, 90) : sf::Color(50, 50, 60));
    exportBtn.setFillColor(exportBtn.getGlobalBounds().contains(mousePos) ? sf::Color(0, 150, 255) : sf::Color(0, 122, 204));
    transCheckbox.setOutlineColor(transCheckbox.getGlobalBounds().contains(mousePos) ? sf::Color::White : sf::Color::Transparent);
    cropCheckbox.setOutlineColor(cropCheckbox.getGlobalBounds().contains(mousePos) ? sf::Color::White : sf::Color::Transparent);
}

void ExportModal::handleEvent(const sf::Event& event, sf::RenderWindow& window) {
    if (!isOpen) return;

    sf::Vector2f mousePos(static_cast<float>(sf::Mouse::getPosition(window).x), static_cast<float>(sf::Mouse::getPosition(window).y));

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
        else if (exportBtn.getGlobalBounds().contains(mousePos)) {
            ExportManager::exportSingleImage(*linkedCanvas, activeFrame, "export_preview.png", transparentBg, autoCrop);
            close();
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
    window.draw(exportBtn);
    window.draw(exportText);

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