#include "NewProjectModal.h"
#include <algorithm>

const int NORMAL_W[] = { 640, 800, 1280, 1920, 0 };
const int NORMAL_H[] = { 360, 600, 720, 1080, 0 };
const std::string NORMAL_LABELS[] = { "640x360", "800x600", "1280x720", "1920x1080", "Custom" };

const int PIXEL_W[] = { 16, 32, 64, 128, 256, 0 };
const int PIXEL_H[] = { 16, 32, 64, 128, 256, 0 };
const std::string PIXEL_LABELS[] = { "16x16", "32x32", "64x64", "128x128", "256x256", "Custom" };

NewProjectModal::NewProjectModal() : isOpen(false), isPixelMode(false), selectedPresetIndex(2), customWidth(1280), customHeight(720), typingWidth(false), typingHeight(false), projectName("") {}

void NewProjectModal::init() {
    font.loadFromFile("assets/font.otf");

    overlay.setSize(sf::Vector2f(1920.f, 1080.f));
    overlay.setFillColor(sf::Color(0, 0, 0, 200));

    modalBg.setSize(sf::Vector2f(800.f, 500.f));
    modalBg.setPosition(1920.f / 2.f - 400.f, 1080.f / 2.f - 250.f);
    modalBg.setFillColor(sf::Color(25, 25, 30, 255));
    modalBg.setOutlineThickness(2.f);
    modalBg.setOutlineColor(sf::Color(100, 100, 110, 100));

    title.setFont(font);
    title.setString("Create New Project");
    title.setCharacterSize(24);
    title.setFillColor(sf::Color::White);
    title.setPosition(modalBg.getPosition().x + 30.f, modalBg.getPosition().y + 30.f);

    closeBtn.setSize(sf::Vector2f(100.f, 40.f));
    closeBtn.setPosition(modalBg.getPosition().x + 670.f, modalBg.getPosition().y + 30.f);
    closeBtn.setFillColor(sf::Color(50, 50, 60));

    closeText.setFont(font);
    closeText.setString("Cancel");
    closeText.setCharacterSize(16);
    closeText.setFillColor(sf::Color::White);
    closeText.setPosition(closeBtn.getPosition().x + 25.f, closeBtn.getPosition().y + 10.f);

    normalToggleBtn.setSize(sf::Vector2f(150.f, 40.f));
    normalToggleBtn.setPosition(modalBg.getPosition().x + 30.f, modalBg.getPosition().y + 100.f);
    normalToggleText.setFont(font);
    normalToggleText.setString("Normal");
    normalToggleText.setCharacterSize(18);
    normalToggleText.setPosition(normalToggleBtn.getPosition().x + 40.f, normalToggleBtn.getPosition().y + 10.f);

    pixelToggleBtn.setSize(sf::Vector2f(150.f, 40.f));
    pixelToggleBtn.setPosition(modalBg.getPosition().x + 190.f, modalBg.getPosition().y + 100.f);
    pixelToggleText.setFont(font);
    pixelToggleText.setString("Pixel Art");
    pixelToggleText.setCharacterSize(18);
    pixelToggleText.setPosition(pixelToggleBtn.getPosition().x + 35.f, pixelToggleBtn.getPosition().y + 10.f);

    widthLabel.setFont(font); widthLabel.setString("Width:"); widthLabel.setCharacterSize(18); widthLabel.setFillColor(sf::Color::White);
    widthLabel.setPosition(modalBg.getPosition().x + 30.f, modalBg.getPosition().y + 300.f);
    widthBg.setSize(sf::Vector2f(120.f, 35.f)); widthBg.setPosition(modalBg.getPosition().x + 100.f, modalBg.getPosition().y + 295.f);
    widthText.setFont(font); widthText.setCharacterSize(18); widthText.setPosition(widthBg.getPosition().x + 10.f, widthBg.getPosition().y + 5.f);

    heightLabel.setFont(font); heightLabel.setString("Height:"); heightLabel.setCharacterSize(18); heightLabel.setFillColor(sf::Color::White);
    heightLabel.setPosition(modalBg.getPosition().x + 250.f, modalBg.getPosition().y + 300.f);
    heightBg.setSize(sf::Vector2f(120.f, 35.f)); heightBg.setPosition(modalBg.getPosition().x + 330.f, modalBg.getPosition().y + 295.f);
    heightText.setFont(font); heightText.setCharacterSize(18); heightText.setPosition(heightBg.getPosition().x + 10.f, heightBg.getPosition().y + 5.f);

    createBtn.setSize(sf::Vector2f(200.f, 50.f));
    createBtn.setPosition(modalBg.getPosition().x + 570.f, modalBg.getPosition().y + 420.f);
    createBtn.setFillColor(sf::Color(50, 180, 50));
    createText.setFont(font);
    createText.setString("Create");
    createText.setCharacterSize(22);
    createText.setFillColor(sf::Color::White);
    createText.setPosition(createBtn.getPosition().x + 65.f, createBtn.getPosition().y + 12.f);

    buildPresets();
    updateSelectionVisuals();
}

void NewProjectModal::buildPresets() {
    presetBtns.clear();
    presetTexts.clear();

    int count = isPixelMode ? 6 : 5;
    const std::string* labels = isPixelMode ? PIXEL_LABELS : NORMAL_LABELS;

    float px = modalBg.getPosition().x + 30.f;
    float py = modalBg.getPosition().y + 170.f;

    for (int i = 0; i < count; i++) {
        sf::RectangleShape btn(sf::Vector2f(120.f, 40.f));
        btn.setPosition(px, py);
        presetBtns.push_back(btn);

        sf::Text txt(labels[i], font, 16);
        txt.setPosition(px + 10.f, py + 10.f);
        presetTexts.push_back(txt);

        px += 130.f;
        if (px > modalBg.getPosition().x + 600.f) {
            px = modalBg.getPosition().x + 30.f;
            py += 50.f;
        }
    }
}

void NewProjectModal::updateSelectionVisuals() {
    normalToggleBtn.setFillColor(isPixelMode ? sf::Color(40, 40, 50) : sf::Color(80, 120, 200));
    pixelToggleBtn.setFillColor(isPixelMode ? sf::Color(80, 120, 200) : sf::Color(40, 40, 50));

    for (size_t i = 0; i < presetBtns.size(); i++) {
        presetBtns[i].setFillColor((static_cast<int>(i) == selectedPresetIndex) ? sf::Color(100, 150, 220) : sf::Color(50, 50, 60));
    }

    widthBg.setFillColor(typingWidth ? sf::Color(60, 60, 80) : sf::Color(30, 30, 40));
    heightBg.setFillColor(typingHeight ? sf::Color(60, 60, 80) : sf::Color(30, 30, 40));
    widthBg.setOutlineThickness(1.f); widthBg.setOutlineColor(sf::Color(100, 100, 110));
    heightBg.setOutlineThickness(1.f); heightBg.setOutlineColor(sf::Color(100, 100, 110));

    widthText.setString(std::to_string(customWidth) + (typingWidth ? "_" : ""));
    heightText.setString(std::to_string(customHeight) + (typingHeight ? "_" : ""));
}

void NewProjectModal::open() {
    isOpen = true;
    typingWidth = false;
    typingHeight = false;
    updateSelectionVisuals();
}

void NewProjectModal::close() { isOpen = false; }
bool NewProjectModal::getIsOpen() const { return isOpen; }

void NewProjectModal::updateHover(sf::Vector2f mousePos) {
    if (!isOpen) return;
    closeBtn.setFillColor(closeBtn.getGlobalBounds().contains(mousePos) ? sf::Color(80, 80, 90) : sf::Color(50, 50, 60));
    createBtn.setFillColor(createBtn.getGlobalBounds().contains(mousePos) ? sf::Color(70, 200, 70) : sf::Color(50, 180, 50));
}

std::string NewProjectModal::handleEvent(const sf::Event& event, sf::RenderWindow& window) {
    if (!isOpen) return "";

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {

        // FIX: Map mouse pixels to windowed coords
        sf::Vector2i pixelPos = sf::Mouse::getPosition(window);
        sf::Vector2f mousePos = window.mapPixelToCoords(pixelPos);

        if (closeBtn.getGlobalBounds().contains(mousePos)) { close(); return "cancel"; }
        if (createBtn.getGlobalBounds().contains(mousePos)) {
            customWidth = std::max(1, customWidth);
            customHeight = std::max(1, customHeight);
            close();
            return "create";
        }

        if (normalToggleBtn.getGlobalBounds().contains(mousePos) && isPixelMode) {
            isPixelMode = false;
            selectedPresetIndex = 2;
            customWidth = NORMAL_W[selectedPresetIndex];
            customHeight = NORMAL_H[selectedPresetIndex];
            buildPresets();
            updateSelectionVisuals();
            return "";
        }
        if (pixelToggleBtn.getGlobalBounds().contains(mousePos) && !isPixelMode) {
            isPixelMode = true;
            selectedPresetIndex = 2;
            customWidth = PIXEL_W[selectedPresetIndex];
            customHeight = PIXEL_H[selectedPresetIndex];
            buildPresets();
            updateSelectionVisuals();
            return "";
        }

        for (size_t i = 0; i < presetBtns.size(); i++) {
            if (presetBtns[i].getGlobalBounds().contains(mousePos)) {
                selectedPresetIndex = static_cast<int>(i);
                int w = isPixelMode ? PIXEL_W[i] : NORMAL_W[i];
                int h = isPixelMode ? PIXEL_H[i] : NORMAL_H[i];
                if (w != 0 && h != 0) {
                    customWidth = w;
                    customHeight = h;
                }
                typingWidth = false; typingHeight = false;
                updateSelectionVisuals();
                return "";
            }
        }

        typingWidth = widthBg.getGlobalBounds().contains(mousePos);
        typingHeight = heightBg.getGlobalBounds().contains(mousePos);
        if (typingWidth || typingHeight) {
            selectedPresetIndex = isPixelMode ? 5 : 4;
        }
        updateSelectionVisuals();
    }

    if (event.type == sf::Event::TextEntered) {
        if (typingWidth || typingHeight) {
            if (event.text.unicode == '\b') {
                if (typingWidth) customWidth /= 10;
                if (typingHeight) customHeight /= 10;
            }
            else if (event.text.unicode >= '0' && event.text.unicode <= '9') {
                int digit = event.text.unicode - '0';
                if (typingWidth) customWidth = std::min(16384, customWidth * 10 + digit);
                if (typingHeight) customHeight = std::min(16384, customHeight * 10 + digit);
            }
            updateSelectionVisuals();
        }
    }

    return "";
}

void NewProjectModal::draw(sf::RenderWindow& window) {
    if (!isOpen) return;
    window.draw(overlay);
    window.draw(modalBg);
    window.draw(title);
    window.draw(closeBtn);
    window.draw(closeText);

    window.draw(normalToggleBtn); window.draw(normalToggleText);
    window.draw(pixelToggleBtn); window.draw(pixelToggleText);

    for (size_t i = 0; i < presetBtns.size(); i++) {
        window.draw(presetBtns[i]);
        window.draw(presetTexts[i]);
    }

    window.draw(widthLabel); window.draw(widthBg); window.draw(widthText);
    window.draw(heightLabel); window.draw(heightBg); window.draw(heightText);

    window.draw(createBtn);
    window.draw(createText);
}