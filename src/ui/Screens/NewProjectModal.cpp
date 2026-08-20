#include "NewProjectModal.h"
#include "../UITheme.h"
#include <algorithm>

const int NORMAL_W[] = { 640, 800, 1280, 1920, 0 };
const int NORMAL_H[] = { 360, 600, 720, 1080, 0 };
const std::string NORMAL_LABELS[] = { "640x360", "800x600", "1280x720", "1920x1080", "Custom" };

const int PIXEL_W[] = { 16, 32, 64, 128, 256, 0 };
const int PIXEL_H[] = { 16, 32, 64, 128, 256, 0 };
const std::string PIXEL_LABELS[] = { "16x16", "32x32", "64x64", "128x128", "256x256", "Custom" };

NewProjectModal::NewProjectModal()
    : isOpen(false), isPixelMode(false), selectedPresetIndex(2),
    customWidth(1280), customHeight(720), typingWidth(false),
    typingHeight(false), typingName(false), projectName("") {}

void NewProjectModal::init() {
    font.loadFromFile("assets/font.otf");

    overlay.setSize(sf::Vector2f(1920.f, 1080.f));
    overlay.setFillColor(sf::Color(10, 4, 16, 220));

    modalBounds = sf::FloatRect(1920.f / 2.f - 430.f, 1080.f / 2.f - 280.f, 860.f, 560.f);

    closeBtnBounds = sf::FloatRect(modalBounds.left + modalBounds.width - 104.f, modalBounds.top + 20.f, 84.f, 32.f);

    normalToggleBounds = sf::FloatRect(modalBounds.left + 30.f, modalBounds.top + 80.f, 180.f, 38.f);
    pixelToggleBounds = sf::FloatRect(modalBounds.left + 220.f, modalBounds.top + 80.f, 180.f, 38.f);

    nameInputBounds = sf::FloatRect(modalBounds.left + 30.f, modalBounds.top + 330.f, 380.f, 44.f);
    widthInputBounds = sf::FloatRect(modalBounds.left + 30.f, modalBounds.top + 410.f, 180.f, 44.f);
    heightInputBounds = sf::FloatRect(modalBounds.left + 230.f, modalBounds.top + 410.f, 180.f, 44.f);

    createBtnBounds = sf::FloatRect(modalBounds.left + modalBounds.width - 240.f, modalBounds.top + modalBounds.height - 74.f, 210.f, 50.f);

    buildPresets();
}

void NewProjectModal::buildPresets() {
    presetBounds.clear();

    int count = isPixelMode ? 6 : 5;
    float px = modalBounds.left + 30.f;
    float py = modalBounds.top + 150.f;

    for (int i = 0; i < count; i++) {
        presetBounds.push_back(sf::FloatRect(px, py, 126.f, 42.f));
        px += 134.f;
        if (px > modalBounds.left + 700.f) {
            px = modalBounds.left + 30.f;
            py += 52.f;
        }
    }
}

void NewProjectModal::open() {
    isOpen = true;
    typingWidth = false;
    typingHeight = false;
    typingName = false;
    buildPresets();
}

void NewProjectModal::close() {
    isOpen = false;
}

bool NewProjectModal::getIsOpen() const {
    return isOpen;
}

void NewProjectModal::updateHover(sf::Vector2f mousePos) {}

std::string NewProjectModal::handleEvent(const sf::Event& event, sf::RenderWindow& window) {
    if (!isOpen) return "";

    sf::Vector2i pixelPos = sf::Mouse::getPosition(window);
    sf::Vector2f mousePos = window.mapPixelToCoords(pixelPos);

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        if (closeBtnBounds.contains(mousePos)) {
            close();
            return "cancel";
        }
        if (createBtnBounds.contains(mousePos)) {
            customWidth = std::max(1, customWidth);
            customHeight = std::max(1, customHeight);
            close();
            return "create";
        }

        if (normalToggleBounds.contains(mousePos) && isPixelMode) {
            isPixelMode = false;
            selectedPresetIndex = 2;
            customWidth = NORMAL_W[selectedPresetIndex];
            customHeight = NORMAL_H[selectedPresetIndex];
            buildPresets();
            return "";
        }
        if (pixelToggleBounds.contains(mousePos) && !isPixelMode) {
            isPixelMode = true;
            selectedPresetIndex = 2;
            customWidth = PIXEL_W[selectedPresetIndex];
            customHeight = PIXEL_H[selectedPresetIndex];
            buildPresets();
            return "";
        }

        for (size_t i = 0; i < presetBounds.size(); i++) {
            if (presetBounds[i].contains(mousePos)) {
                selectedPresetIndex = static_cast<int>(i);
                int w = isPixelMode ? PIXEL_W[i] : NORMAL_W[i];
                int h = isPixelMode ? PIXEL_H[i] : NORMAL_H[i];
                if (w != 0 && h != 0) {
                    customWidth = w;
                    customHeight = h;
                }
                typingWidth = false;
                typingHeight = false;
                typingName = false;
                return "";
            }
        }

        typingName = nameInputBounds.contains(mousePos);
        typingWidth = widthInputBounds.contains(mousePos);
        typingHeight = heightInputBounds.contains(mousePos);

        if (typingWidth || typingHeight) {
            selectedPresetIndex = isPixelMode ? 5 : 4;
        }
    }

    if (event.type == sf::Event::TextEntered) {
        if (typingName) {
            if (event.text.unicode == '\b' && !projectName.empty()) projectName.pop_back();
            else if (event.text.unicode >= 32 && event.text.unicode < 127 && projectName.length() < 24) {
                projectName += static_cast<char>(event.text.unicode);
            }
        }
        else if (typingWidth || typingHeight) {
            if (event.text.unicode == '\b') {
                if (typingWidth) customWidth /= 10;
                if (typingHeight) customHeight /= 10;
            }
            else if (event.text.unicode >= '0' && event.text.unicode <= '9') {
                int digit = event.text.unicode - '0';
                if (typingWidth) customWidth = std::min(16384, customWidth * 10 + digit);
                if (typingHeight) customHeight = std::min(16384, customHeight * 10 + digit);
            }
        }
    }

    return "";
}

void NewProjectModal::draw(sf::RenderWindow& window) {
    if (!isOpen) return;

    window.draw(overlay);

    WisdomUI::Theme::DrawSunsetPanel(window, modalBounds, 1.0f);

    WisdomUI::Theme::DrawCrispText(window, font, ":: INITIALIZE NEW CANVAS ::", 14, modalBounds.left + 30.f, modalBounds.top + 28.f, WisdomUI::Theme::SunsetAmber, sf::Color(14, 6, 20));

    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

    WisdomUI::Theme::DrawSunsetButton(window, closeBtnBounds, "Cancel", font, 11, false, closeBtnBounds.contains(mPos), false, 1.0f);

    WisdomUI::Theme::DrawSunsetButton(window, normalToggleBounds, "Standard Dynamic", font, 12, !isPixelMode, normalToggleBounds.contains(mPos), !isPixelMode, 1.0f);
    WisdomUI::Theme::DrawSunsetButton(window, pixelToggleBounds, "Pixel Art Grid", font, 12, isPixelMode, pixelToggleBounds.contains(mPos), isPixelMode, 1.0f);

    const std::string* labels = isPixelMode ? PIXEL_LABELS : NORMAL_LABELS;
    for (size_t i = 0; i < presetBounds.size(); i++) {
        bool isSel = (static_cast<int>(i) == selectedPresetIndex);
        WisdomUI::Theme::DrawSunsetButton(window, presetBounds[i], labels[i], font, 12, isSel, presetBounds[i].contains(mPos), isSel, 1.0f);
    }

    auto drawField = [&](sf::FloatRect b, const std::string& label, const std::string& val, bool active, const std::string& placeholder) {
        sf::RectangleShape box(sf::Vector2f(b.width, b.height));
        box.setPosition(b.left, b.top);
        box.setFillColor(WisdomUI::Theme::SunsetDeepDark);
        box.setOutlineThickness(1.5f);
        box.setOutlineColor(active ? WisdomUI::Theme::SunsetGold : WisdomUI::Theme::SunsetPlum);
        window.draw(box);

        WisdomUI::Theme::DrawCrispText(window, font, label, 10, b.left, b.top - 18.f, WisdomUI::Theme::TextSecondary);

        std::string display = val;
        if (active) display += "_";
        else if (display.empty()) display = placeholder;

        sf::Color textColor = (val.empty() && !active) ? WisdomUI::Theme::SunsetPlum : WisdomUI::Theme::TextPrimary;
        WisdomUI::Theme::DrawCrispText(window, font, display, 13, b.left + 12.f, b.top + 12.f, textColor);
        };

    drawField(nameInputBounds, "PROJECT IDENTITY", projectName, typingName, "Untitled_Artwork");
    drawField(widthInputBounds, "CANVAS WIDTH (PX)", std::to_string(customWidth), typingWidth, "Width");
    drawField(heightInputBounds, "CANVAS HEIGHT (PX)", std::to_string(customHeight), typingHeight, "Height");

    WisdomUI::Theme::DrawSunsetButton(window, createBtnBounds, "CREATE PROJECT", font, 13, false, createBtnBounds.contains(mPos), true, 1.0f);
}