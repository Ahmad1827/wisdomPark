#include "NewProjectModal.h"
#include "../UITheme.h"
#include <algorithm>

const int PRESET_RETRO_W[] = { 16, 32, 64, 128, 256, 320 };
const int PRESET_RETRO_H[] = { 16, 32, 64, 128, 256, 180 };
const std::string PRESET_RETRO_LABELS[] = { "16x16", "32x32", "64x64", "128x128", "256x256", "320x180" };

const int PRESET_DYNAMIC_W[] = { 640, 800, 1280, 1920, 2560, 3840 };
const int PRESET_DYNAMIC_H[] = { 360, 600, 720, 1080, 1440, 2160 };
const std::string PRESET_DYNAMIC_LABELS[] = { "640x360", "800x600", "1280x720", "1920x1080", "2560x1440", "3840x2160" };

NewProjectModal::NewProjectModal()
    : isOpen(false), isPixelMode(false), selectedPresetIndex(2),
    customWidth(1280), customHeight(720), typingWidth(false),
    typingHeight(false), typingName(false), projectName("") {}

void NewProjectModal::init() {
    font.loadFromFile("assets/font.otf");

    overlay.setSize(sf::Vector2f(1920.f, 1080.f));
    overlay.setFillColor(sf::Color(10, 4, 16, 225));

    modalBounds = sf::FloatRect(1920.f / 2.f - 520.f, 1080.f / 2.f - 340.f, 1040.f, 680.f);

    closeBtnBounds = sf::FloatRect(modalBounds.left + modalBounds.width - 130.f, modalBounds.top + 22.f, 100.f, 42.f);

    normalToggleBounds = sf::FloatRect(modalBounds.left + 36.f, modalBounds.top + 84.f, 280.f, 48.f);
    pixelToggleBounds = sf::FloatRect(modalBounds.left + 332.f, modalBounds.top + 84.f, 280.f, 48.f);

    nameInputBounds = sf::FloatRect(modalBounds.left + 36.f, modalBounds.top + 370.f, 576.f, 50.f);
    widthInputBounds = sf::FloatRect(modalBounds.left + 36.f, modalBounds.top + 460.f, 276.f, 50.f);
    heightInputBounds = sf::FloatRect(modalBounds.left + 336.f, modalBounds.top + 460.f, 276.f, 50.f);

    previewFrameBounds = sf::FloatRect(modalBounds.left + 648.f, modalBounds.top + 84.f, 356.f, 426.f);
    createBtnBounds = sf::FloatRect(modalBounds.left + 648.f, modalBounds.top + 536.f, 356.f, 64.f);

    buildPresets();
}

void NewProjectModal::buildPresets() {
    presetBounds.clear();

    float px = modalBounds.left + 36.f;
    float py = modalBounds.top + 152.f;

    for (int i = 0; i < 6; i++) {
        presetBounds.push_back(sf::FloatRect(px, py, 180.f, 52.f));
        px += 198.f;
        if (px > modalBounds.left + 450.f) {
            px = modalBounds.left + 36.f;
            py += 64.f;
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
            customWidth = std::clamp(customWidth, 8, 16384);
            customHeight = std::clamp(customHeight, 8, 16384);
            close();
            return "create";
        }

        if (normalToggleBounds.contains(mousePos) && isPixelMode) {
            isPixelMode = false;
            selectedPresetIndex = 2;
            customWidth = PRESET_DYNAMIC_W[selectedPresetIndex];
            customHeight = PRESET_DYNAMIC_H[selectedPresetIndex];
            buildPresets();
            return "";
        }
        if (pixelToggleBounds.contains(mousePos) && !isPixelMode) {
            isPixelMode = true;
            selectedPresetIndex = 2;
            customWidth = PRESET_RETRO_W[selectedPresetIndex];
            customHeight = PRESET_RETRO_H[selectedPresetIndex];
            buildPresets();
            return "";
        }

        for (size_t i = 0; i < presetBounds.size(); i++) {
            if (presetBounds[i].contains(mousePos)) {
                selectedPresetIndex = static_cast<int>(i);
                customWidth = isPixelMode ? PRESET_RETRO_W[i] : PRESET_DYNAMIC_W[i];
                customHeight = isPixelMode ? PRESET_RETRO_H[i] : PRESET_DYNAMIC_H[i];
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
            selectedPresetIndex = -1;
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

    WisdomUI::Theme::DrawCrispText(window, font, "NEW PROJECT", 32, modalBounds.left + 36.f, modalBounds.top + 24.f, WisdomUI::Theme::SunsetGold, sf::Color(14, 6, 20));

    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

    WisdomUI::Theme::DrawSunsetButton(window, closeBtnBounds, "Cancel", font, 18, false, closeBtnBounds.contains(mPos), false, 1.0f);

    WisdomUI::Theme::DrawSunsetButton(window, normalToggleBounds, "Standard Dynamic", font, 18, !isPixelMode, normalToggleBounds.contains(mPos), !isPixelMode, 1.0f);
    WisdomUI::Theme::DrawSunsetButton(window, pixelToggleBounds, "Pixel Art Grid", font, 18, isPixelMode, pixelToggleBounds.contains(mPos), isPixelMode, 1.0f);

    const std::string* labels = isPixelMode ? PRESET_RETRO_LABELS : PRESET_DYNAMIC_LABELS;
    for (size_t i = 0; i < presetBounds.size(); i++) {
        bool isSel = (static_cast<int>(i) == selectedPresetIndex);
        WisdomUI::Theme::DrawSunsetButton(window, presetBounds[i], labels[i], font, 17, isSel, presetBounds[i].contains(mPos), isSel, 1.0f);
    }

    auto drawField = [&](sf::FloatRect b, const std::string& label, const std::string& val, bool active, const std::string& placeholder) {
        sf::RectangleShape box(sf::Vector2f(b.width, b.height));
        box.setPosition(b.left, b.top);
        box.setFillColor(WisdomUI::Theme::SunsetDeepDark);
        box.setOutlineThickness(1.5f);
        box.setOutlineColor(active ? WisdomUI::Theme::SunsetGold : WisdomUI::Theme::SunsetPlum);
        window.draw(box);

        WisdomUI::Theme::DrawCrispText(window, font, label, 16, b.left, b.top - 24.f, WisdomUI::Theme::TextSecondary);

        std::string display = val;
        if (active) display += "_";
        else if (display.empty()) display = placeholder;

        sf::Color textColor = (val.empty() && !active) ? WisdomUI::Theme::SunsetPlum : WisdomUI::Theme::TextPrimary;
        WisdomUI::Theme::DrawCrispText(window, font, display, 20, b.left + 16.f, b.top + 13.f, textColor);
        };

    drawField(nameInputBounds, "Project Name", projectName, typingName, "Untitled_Artwork");
    drawField(widthInputBounds, "Width (px)", std::to_string(customWidth), typingWidth, "Width");
    drawField(heightInputBounds, "Height (px)", std::to_string(customHeight), typingHeight, "Height");

    sf::RectangleShape previewFrame(sf::Vector2f(previewFrameBounds.width, previewFrameBounds.height));
    previewFrame.setPosition(previewFrameBounds.left, previewFrameBounds.top);
    previewFrame.setFillColor(WisdomUI::Theme::SunsetDeepDark);
    previewFrame.setOutlineThickness(1.5f);
    previewFrame.setOutlineColor(WisdomUI::Theme::SunsetPlum);
    window.draw(previewFrame);

    WisdomUI::Theme::DrawCrispText(window, font, "CANVAS RATIO", 18, previewFrameBounds.left + 20.f, previewFrameBounds.top + 16.f, WisdomUI::Theme::SunsetAmber);

    float maxVisualW = previewFrameBounds.width - 48.f;
    float maxVisualH = previewFrameBounds.height - 110.f;
    float ratioScale = std::min(maxVisualW / static_cast<float>(std::max(1, customWidth)), maxVisualH / static_cast<float>(std::max(1, customHeight)));
    float visW = std::max(16.f, static_cast<float>(customWidth) * ratioScale);
    float visH = std::max(16.f, static_cast<float>(customHeight) * ratioScale);

    sf::RectangleShape visualCanvas(sf::Vector2f(visW, visH));
    visualCanvas.setOrigin(visW / 2.f, visH / 2.f);
    visualCanvas.setPosition(previewFrameBounds.left + previewFrameBounds.width / 2.f, previewFrameBounds.top + 50.f + maxVisualH / 2.f);
    visualCanvas.setFillColor(WisdomUI::Theme::SunsetSkyTop);
    visualCanvas.setOutlineThickness(1.5f);
    visualCanvas.setOutlineColor(WisdomUI::Theme::SunsetGold);
    window.draw(visualCanvas);

    std::string ratioInfo = std::to_string(customWidth) + " x " + std::to_string(customHeight);
    WisdomUI::Theme::DrawCrispText(window, font, ratioInfo, 18, previewFrameBounds.left + previewFrameBounds.width / 2.f, previewFrameBounds.top + previewFrameBounds.height - 34.f, WisdomUI::Theme::SunsetPeach, sf::Color::Transparent, true, true);

    WisdomUI::Theme::DrawSunsetButton(window, createBtnBounds, "CREATE PROJECT", font, 22, false, createBtnBounds.contains(mPos), true, 1.0f);
}