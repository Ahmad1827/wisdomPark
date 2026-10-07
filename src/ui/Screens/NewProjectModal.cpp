#include "NewProjectModal.h"
#include "../UITheme.h"
#include "MenuWidgets.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numeric>

using WisdomUI::Theme;
using WisdomUI::Animation;
using namespace MenuWidgets;

namespace {

    struct Preset {
        int width;
        int height;
        const char* label;
        const char* note;
    };

    const int kPresetCount = 6;
    const int kMinSize = 8;
    const int kMaxSize = 16384;
    const size_t kMaxNameLength = 24;

    const Preset kRetroPresets[kPresetCount] = {
        { 16, 16, "16 x 16", "TINY ICON" },
        { 32, 32, "32 x 32", "SPRITE" },
        { 64, 64, "64 x 64", "CHARACTER" },
        { 128, 128, "128 x 128", "PORTRAIT" },
        { 256, 256, "256 x 256", "SCENE" },
        { 320, 180, "320 x 180", "16:9 SCREEN" }
    };

    const Preset kDynamicPresets[kPresetCount] = {
        { 640, 360, "640 x 360", "SKETCH" },
        { 800, 600, "800 x 600", "CLASSIC 4:3" },
        { 1280, 720, "1280 x 720", "HD 720P" },
        { 1920, 1080, "1920 x 1080", "FULL HD" },
        { 2560, 1440, "2560 x 1440", "QHD 1440P" },
        { 3840, 2160, "3840 x 2160", "4K ULTRA HD" }
    };

    const Preset* presetsFor(bool pixelMode) {
        return pixelMode ? kRetroPresets : kDynamicPresets;
    }

    std::string aspectLabel(int width, int height) {
        int divisor = std::gcd(width, height);
        int rw = width / divisor;
        int rh = height / divisor;
        if (rw <= 64 && rh <= 64) return std::to_string(rw) + ":" + std::to_string(rh);

        char buf[24];
        std::snprintf(buf, sizeof(buf), "%.2f:1", static_cast<float>(width) / static_cast<float>(height));
        return buf;
    }

    std::string pixelCountLabel(int width, int height) {
        long long pixels = static_cast<long long>(width) * static_cast<long long>(height);
        if (pixels < 1000000) return std::to_string(pixels) + " PX";

        char buf[24];
        std::snprintf(buf, sizeof(buf), "%.1f MP", static_cast<double>(pixels) / 1000000.0);
        return buf;
    }

    // Selectable tile used for the canvas type and the size presets.
    void drawOptionTile(sf::RenderWindow& window, const sf::FloatRect& b, bool selected, float hoverT) {
        float lit = std::max(hoverT, selected ? 0.55f : 0.0f);

        if (selected || hoverT > 0.01f) {
            float glowAlpha = selected ? (44.0f + 18.0f * std::sin(Theme::s_time * 3.0f)) : (46.0f * hoverT);
            sf::ConvexShape glow = Theme::ChamferedRect(b.left - 3.0f, b.top - 3.0f, b.width + 6.0f, b.height + 6.0f, 6.0f);
            glow.setFillColor(Theme::WithAlpha(selected ? Theme::SunsetAmber : Theme::SunsetPeach, glowAlpha));
            window.draw(glow);
        }

        sf::ConvexShape tile = Theme::ChamferedRect(b.left, b.top, b.width, b.height, 4.0f);
        tile.setFillColor(Theme::Mix(sf::Color(22, 14, 36, 240), sf::Color(52, 32, 76, 248), lit));
        tile.setOutlineThickness(1.0f);
        tile.setOutlineColor(selected ? Theme::SunsetGold : Theme::Mix(Theme::Border, Theme::SunsetPeach, hoverT));
        window.draw(tile);

        sf::RectangleShape shine(sf::Vector2f(b.width - 8.0f, 1.0f));
        shine.setPosition(b.left + 4.0f, b.top + 1.0f);
        shine.setFillColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(26.0f + 40.0f * lit)));
        window.draw(shine);

        if (selected) {
            sf::RectangleShape bar(sf::Vector2f(3.0f, b.height - 8.0f));
            bar.setPosition(b.left + 1.0f, b.top + 4.0f);
            bar.setFillColor(Theme::SunsetAmber);
            window.draw(bar);
        }
    }

    void drawCaption(sf::RenderWindow& window, const sf::Font& font, const std::string& text, float x, float y, float width) {
        Theme::DrawCrispText(window, font, text, 14, x, y, Theme::SunsetAmber, kTextShadow, false, true);

        float textW = Theme::MeasureText(font, text, 14);
        sf::RectangleShape rule(sf::Vector2f(std::max(0.0f, width - textW - 12.0f), 1.0f));
        rule.setPosition(x + textW + 12.0f, y);
        rule.setFillColor(Theme::SunsetPlum);
        window.draw(rule);
    }

}

NewProjectModal::NewProjectModal()
    : isOpen(false), isPixelMode(false), selectedPresetIndex(2),
    customWidth(1280), customHeight(720), typingWidth(false),
    typingHeight(false), typingName(false), projectName(""),
    openTime(0.0f), shownWidth(1280.0f), shownHeight(720.0f) {}

void NewProjectModal::init() {
    font.loadFromFile("assets/font.otf");

    modalBounds = sf::FloatRect(1920.f / 2.f - 520.f, 1080.f / 2.f - 330.f, 1040.f, 660.f);

    closeBtnBounds = sf::FloatRect(modalBounds.left + modalBounds.width - 150.f, modalBounds.top + 32.f, 110.f, 42.f);

    normalToggleBounds = sf::FloatRect(modalBounds.left + 40.f, modalBounds.top + 150.f, 280.f, 58.f);
    pixelToggleBounds = sf::FloatRect(modalBounds.left + 336.f, modalBounds.top + 150.f, 280.f, 58.f);

    nameInputBounds = sf::FloatRect(modalBounds.left + 40.f, modalBounds.top + 438.f, 576.f, 48.f);
    widthInputBounds = sf::FloatRect(modalBounds.left + 40.f, modalBounds.top + 524.f, 276.f, 48.f);
    heightInputBounds = sf::FloatRect(modalBounds.left + 340.f, modalBounds.top + 524.f, 276.f, 48.f);

    previewFrameBounds = sf::FloatRect(modalBounds.left + 648.f, modalBounds.top + 132.f, 352.f, 400.f);
    createBtnBounds = sf::FloatRect(modalBounds.left + 648.f, modalBounds.top + 548.f, 352.f, 64.f);

    buildPresets();
}

void NewProjectModal::buildPresets() {
    presetBounds.clear();

    for (int i = 0; i < kPresetCount; i++) {
        float col = static_cast<float>(i % 3);
        float row = static_cast<float>(i / 3);
        presetBounds.push_back(sf::FloatRect(modalBounds.left + 40.f + col * 198.f, modalBounds.top + 248.f + row * 74.f, 180.f, 62.f));
    }
}

void NewProjectModal::applyPreset(int index) {
    const Preset& preset = presetsFor(isPixelMode)[index];
    selectedPresetIndex = index;
    customWidth = preset.width;
    customHeight = preset.height;
}

// Keeps the highlighted preset honest after the size is typed by hand.
void NewProjectModal::syncPresetToSize() {
    const Preset* presets = presetsFor(isPixelMode);
    selectedPresetIndex = -1;
    for (int i = 0; i < kPresetCount; i++) {
        if (presets[i].width == customWidth && presets[i].height == customHeight) selectedPresetIndex = i;
    }
}

std::string NewProjectModal::confirm() {
    customWidth = std::clamp(customWidth, kMinSize, kMaxSize);
    customHeight = std::clamp(customHeight, kMinSize, kMaxSize);
    close();
    return "create";
}

void NewProjectModal::open() {
    isOpen = true;
    typingWidth = false;
    typingHeight = false;
    typingName = false;
    openTime = Theme::s_time;
    shownWidth = static_cast<float>(customWidth);
    shownHeight = static_cast<float>(customHeight);
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

    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Escape) {
            close();
            return "cancel";
        }
        if (event.key.code == sf::Keyboard::Enter) {
            return confirm();
        }
        if (event.key.code == sf::Keyboard::Tab) {
            // Name -> Width -> Height, and back again with Shift.
            int field = typingName ? 0 : typingWidth ? 1 : typingHeight ? 2 : -1;
            if (field < 0) field = event.key.shift ? 2 : 0;
            else field = (field + (event.key.shift ? 2 : 1)) % 3;
            typingName = (field == 0);
            typingWidth = (field == 1);
            typingHeight = (field == 2);
            return "";
        }
    }

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        if (closeBtnBounds.contains(mousePos)) {
            close();
            return "cancel";
        }
        if (createBtnBounds.contains(mousePos)) {
            return confirm();
        }

        if (normalToggleBounds.contains(mousePos) && isPixelMode) {
            isPixelMode = false;
            applyPreset(2);
            buildPresets();
            return "";
        }
        if (pixelToggleBounds.contains(mousePos) && !isPixelMode) {
            isPixelMode = true;
            applyPreset(2);
            buildPresets();
            return "";
        }

        for (size_t i = 0; i < presetBounds.size(); i++) {
            if (presetBounds[i].contains(mousePos)) {
                applyPreset(static_cast<int>(i));
                typingWidth = false;
                typingHeight = false;
                typingName = false;
                return "";
            }
        }

        typingName = nameInputBounds.contains(mousePos);
        typingWidth = widthInputBounds.contains(mousePos);
        typingHeight = heightInputBounds.contains(mousePos);
    }

    if (event.type == sf::Event::TextEntered) {
        if (typingName) {
            if (event.text.unicode == '\b' && !projectName.empty()) projectName.pop_back();
            else if (event.text.unicode >= 32 && event.text.unicode < 127 && projectName.length() < kMaxNameLength) {
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
                if (typingWidth) customWidth = std::min(kMaxSize, customWidth * 10 + digit);
                if (typingHeight) customHeight = std::min(kMaxSize, customHeight * 10 + digit);
            }
            syncPresetToSize();
        }
    }

    return "";
}

void NewProjectModal::draw(sf::RenderWindow& window) {
    if (!isOpen) return;

    float appear = Animation::EaseOutCubic((Theme::s_time - openTime) / 0.22f);

    sf::VertexArray scrim(sf::Quads, 4);
    sf::Color scrimTop = faded(sf::Color(12, 6, 22, 236), appear);
    sf::Color scrimBottom = faded(sf::Color(12, 6, 22, 210), appear);
    scrim[0] = sf::Vertex(sf::Vector2f(0.0f, 0.0f), scrimTop);
    scrim[1] = sf::Vertex(sf::Vector2f(1920.0f, 0.0f), scrimTop);
    scrim[2] = sf::Vertex(sf::Vector2f(1920.0f, 1080.0f), scrimBottom);
    scrim[3] = sf::Vertex(sf::Vector2f(0.0f, 1080.0f), scrimBottom);
    window.draw(scrim);

    // The whole panel rises into place; shifting the view keeps every bound below unchanged.
    sf::View savedView = window.getView();
    sf::View risingView = savedView;
    risingView.move(0.0f, -std::floor(22.0f * (1.0f - appear)));
    window.setView(risingView);

    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    bool mouseDown = sf::Mouse::isButtonPressed(sf::Mouse::Left);

    const float left = modalBounds.left + 40.f;
    const float columnW = 576.f;

    Theme::DrawSunsetPanel(window, modalBounds, appear);

    // ---- Header ----
    Theme::DrawCrispText(window, font, "NEW PROJECT", 34, left, modalBounds.top + 50.f, Theme::SunsetGold, Theme::SunsetCoralDark, false, true);
    Theme::DrawCrispText(window, font, "CHOOSE A CANVAS TYPE AND SIZE", 16, left + 2.f, modalBounds.top + 84.f, Theme::SunsetPeach, kTextShadow, false, true);

    sf::RectangleShape rule(sf::Vector2f(modalBounds.width - 80.f, 1.0f));
    rule.setPosition(left, modalBounds.top + 108.f);
    rule.setFillColor(Theme::SunsetPlum);
    window.draw(rule);

    sf::RectangleShape accent(sf::Vector2f(220.0f * Animation::EaseOutCubic((Theme::s_time - openTime - 0.08f) / 0.4f), 2.0f));
    accent.setPosition(left, modalBounds.top + 107.f);
    accent.setFillColor(Theme::SunsetAmber);
    window.draw(accent);

    Theme::DrawSunsetButton(window, closeBtnBounds, "CANCEL", font, 15, false, closeBtnBounds.contains(mPos), false, 1.0f);

    // ---- Canvas type ----
    drawCaption(window, font, "CANVAS TYPE", left, modalBounds.top + 132.f, columnW);

    auto drawTypeTile = [&](const sf::FloatRect& b, const std::string& title, const std::string& note, bool selected) {
        float t = Theme::AnimateHover(b, b.contains(mPos));
        drawOptionTile(window, b, selected, t);
        Theme::DrawCrispText(window, font, title, 17, b.left + 20.f, b.top + 21.f, selected ? Theme::SunsetGold : Theme::Mix(Theme::TextPrimary, Theme::SunsetGold, t), kTextShadow, false, true);
        Theme::DrawCrispText(window, font, note, 14, b.left + 20.f, b.top + 42.f, selected ? Theme::TextSecondary : Theme::TextMuted, sf::Color::Transparent, false, true);
        if (selected) drawDiamond(window, b.left + b.width - 22.f, b.top + b.height * 0.5f, 5.0f, Theme::SunsetGold);
        };

    drawTypeTile(normalToggleBounds, "Standard Dynamic", "Smooth brushes, large canvas", !isPixelMode);
    drawTypeTile(pixelToggleBounds, "Pixel Art Grid", "Hard pixels, small canvas", isPixelMode);

    // ---- Size presets ----
    drawCaption(window, font, "SIZE PRESETS", left, modalBounds.top + 230.f, columnW);

    const Preset* presets = presetsFor(isPixelMode);
    for (size_t i = 0; i < presetBounds.size(); i++) {
        const sf::FloatRect& b = presetBounds[i];
        const Preset& preset = presets[i];
        bool selected = (static_cast<int>(i) == selectedPresetIndex);
        float t = Theme::AnimateHover(b, b.contains(mPos));
        drawOptionTile(window, b, selected, t);

        // Miniature of the canvas shape.
        float iconScale = 30.0f / static_cast<float>(std::max(preset.width, preset.height));
        float iconW = std::floor(std::max(8.0f, preset.width * iconScale));
        float iconH = std::floor(std::max(8.0f, preset.height * iconScale));
        sf::RectangleShape icon(sf::Vector2f(iconW, iconH));
        icon.setPosition(std::floor(b.left + 31.f - iconW * 0.5f), std::floor(b.top + b.height * 0.5f - iconH * 0.5f));
        icon.setFillColor(Theme::WithAlpha(selected ? Theme::SunsetAmber : Theme::SunsetViolet, selected ? 70.0f : 40.0f + 30.0f * t));
        icon.setOutlineThickness(1.0f);
        icon.setOutlineColor(selected ? Theme::SunsetGold : Theme::Mix(Theme::SunsetViolet, Theme::SunsetPeach, t));
        window.draw(icon);

        Theme::DrawCrispText(window, font, preset.label, 16, b.left + 58.f, b.top + 22.f, selected ? Theme::SunsetGold : Theme::Mix(Theme::TextPrimary, Theme::SunsetGold, t), kTextShadow, false, true);
        Theme::DrawCrispText(window, font, preset.note, 13, b.left + 58.f, b.top + 43.f, selected ? Theme::TextSecondary : Theme::TextMuted, sf::Color::Transparent, false, true);
    }

    // ---- Name and custom size ----
    bool caretOn = std::fmod(Theme::s_time, 1.0f) < 0.55f;

    auto drawField = [&](const sf::FloatRect& b, const std::string& label, const std::string& val, bool active, const std::string& placeholder, const std::string& suffix, sf::Color suffixColor) {
        float t = Theme::AnimateHover(b, b.contains(mPos));

        if (active) {
            sf::RectangleShape glow(sf::Vector2f(b.width + 6.0f, b.height + 6.0f));
            glow.setPosition(b.left - 3.0f, b.top - 3.0f);
            glow.setFillColor(Theme::WithAlpha(Theme::SunsetAmber, 40.0f));
            window.draw(glow);
        }

        sf::RectangleShape box(sf::Vector2f(b.width, b.height));
        box.setPosition(b.left, b.top);
        box.setFillColor(Theme::WithAlpha(Theme::PanelInset, 240.0f));
        box.setOutlineThickness(active ? 1.5f : 1.0f);
        box.setOutlineColor(active ? Theme::SunsetGold : Theme::Mix(Theme::SunsetPlum, Theme::SunsetPeach, t));
        window.draw(box);

        Theme::DrawCrispText(window, font, label, 14, b.left, b.top - 14.f, active ? Theme::SunsetGold : Theme::TextSecondary, sf::Color::Transparent, false, true);

        float centerY = b.top + b.height * 0.5f;
        bool showPlaceholder = val.empty() && !active;
        Theme::DrawCrispText(window, font, showPlaceholder ? placeholder : val, 18, b.left + 16.f, centerY,
            showPlaceholder ? Theme::TextMuted : Theme::TextPrimary, sf::Color::Transparent, false, true);

        if (active && caretOn) {
            sf::RectangleShape caret(sf::Vector2f(2.0f, 22.0f));
            caret.setPosition(std::floor(b.left + 16.f + (val.empty() ? 0.0f : Theme::MeasureText(font, val, 18) + 3.0f)), std::floor(centerY - 11.0f));
            caret.setFillColor(Theme::SunsetGold);
            window.draw(caret);
        }

        if (!suffix.empty()) {
            float suffixW = Theme::MeasureText(font, suffix, 13);
            Theme::DrawCrispText(window, font, suffix, 13, b.left + b.width - 14.f - suffixW, centerY, suffixColor, sf::Color::Transparent, false, true);
        }
        };

    // Sizes outside the allowed range are clamped on create, so say so while typing.
    auto sizeSuffix = [&](int value) { return value < kMinSize ? std::string("MIN ") + std::to_string(kMinSize) : std::string("PX"); };
    auto sizeSuffixColor = [&](int value) { return value < kMinSize ? Theme::SunsetCoral : Theme::TextMuted; };

    std::string nameCount = typingName ? (std::to_string(projectName.length()) + "/" + std::to_string(kMaxNameLength)) : "";
    drawField(nameInputBounds, "PROJECT NAME", projectName, typingName, "Leave empty for an automatic name", nameCount, Theme::TextMuted);
    drawField(widthInputBounds, "WIDTH", customWidth > 0 ? std::to_string(customWidth) : "", typingWidth, "Width", sizeSuffix(customWidth), sizeSuffixColor(customWidth));
    drawField(heightInputBounds, "HEIGHT", customHeight > 0 ? std::to_string(customHeight) : "", typingHeight, "Height", sizeSuffix(customHeight), sizeSuffixColor(customHeight));

    Theme::DrawCrispText(window, font, "x", 16, (widthInputBounds.left + widthInputBounds.width + heightInputBounds.left) * 0.5f, widthInputBounds.top + widthInputBounds.height * 0.5f,
        Theme::TextMuted, sf::Color::Transparent, true, true);

    Theme::DrawCrispText(window, font, "TAB  NEXT FIELD   |   ENTER  CREATE   |   ESC  CANCEL", 14, left, modalBounds.top + 600.f, Theme::TextMuted, sf::Color::Transparent, false, true);

    // ---- Preview ----
    const sf::FloatRect& frame = previewFrameBounds;
    sf::RectangleShape previewFrame(sf::Vector2f(frame.width, frame.height));
    previewFrame.setPosition(frame.left, frame.top);
    previewFrame.setFillColor(Theme::WithAlpha(Theme::PanelInset, 240.0f));
    previewFrame.setOutlineThickness(1.0f);
    previewFrame.setOutlineColor(Theme::SunsetPlum);
    window.draw(previewFrame);

    drawCardTitle(window, font, "PREVIEW", frame, 1.0f);

    int previewW = std::clamp(customWidth, kMinSize, kMaxSize);
    int previewH = std::clamp(customHeight, kMinSize, kMaxSize);

    // The preview eases between sizes instead of snapping.
    float ease = std::min(1.0f, 14.0f * Theme::s_frameDt);
    shownWidth += (static_cast<float>(previewW) - shownWidth) * ease;
    shownHeight += (static_cast<float>(previewH) - shownHeight) * ease;

    sf::FloatRect stage(frame.left + 28.f, frame.top + 58.f, frame.width - 56.f, frame.height - 58.f - 88.f);
    float stageScale = std::min(stage.width / shownWidth, stage.height / shownHeight);
    float visW = std::floor(std::max(10.f, shownWidth * stageScale));
    float visH = std::floor(std::max(10.f, shownHeight * stageScale));
    sf::FloatRect paper(std::floor(stage.left + (stage.width - visW) * 0.5f), std::floor(stage.top + (stage.height - visH) * 0.5f), visW, visH);

    sf::RectangleShape paperShadow(sf::Vector2f(paper.width, paper.height));
    paperShadow.setPosition(paper.left + 4.f, paper.top + 6.f);
    paperShadow.setFillColor(sf::Color(4, 2, 8, 170));
    window.draw(paperShadow);

    // Pixel canvases show a chunky grid so their scale reads at a glance.
    float cell = isPixelMode ? std::clamp(visW / shownWidth, 8.0f, 32.0f) : 14.0f;
    drawCheckerboard(window, paper, cell, 1.0f);

    sf::RectangleShape paperEdge(sf::Vector2f(paper.width, paper.height));
    paperEdge.setPosition(paper.left, paper.top);
    paperEdge.setFillColor(sf::Color::Transparent);
    paperEdge.setOutlineThickness(1.5f);
    paperEdge.setOutlineColor(Theme::SunsetGold);
    window.draw(paperEdge);

    float frameCx = frame.left + frame.width * 0.5f;
    Theme::DrawCrispText(window, font, std::to_string(previewW) + " x " + std::to_string(previewH), 22, frameCx, frame.top + frame.height - 56.f, Theme::SunsetGold, kTextShadow, true, true);
    Theme::DrawCrispText(window, font, aspectLabel(previewW, previewH) + "   |   " + pixelCountLabel(previewW, previewH) + "   |   " + (isPixelMode ? "PIXEL ART" : "STANDARD"),
        14, frameCx, frame.top + frame.height - 26.f, Theme::TextSecondary, sf::Color::Transparent, true, true);

    // ---- Create ----
    // Create is the main action here, so it gets the filled amber treatment.
    {
        bool hovered = createBtnBounds.contains(mPos);
        sf::FloatRect b = shifted(createBtnBounds, 0.0f, (hovered && mouseDown) ? 1.0f : 0.0f);
        float t = Theme::AnimateHover(createBtnBounds, hovered);

        sf::ConvexShape glow = Theme::ChamferedRect(b.left - 4.0f, b.top - 4.0f, b.width + 8.0f, b.height + 8.0f, 7.0f);
        glow.setFillColor(Theme::WithAlpha(Theme::SunsetGold, 26.0f + 12.0f * std::sin(Theme::s_time * 3.0f) + 46.0f * t));
        window.draw(glow);

        sf::ConvexShape body = Theme::ChamferedRect(b.left, b.top, b.width, b.height, 4.0f);
        body.setFillColor(Theme::Mix(Theme::SunsetAmber, Theme::SunsetGold, t));
        body.setOutlineThickness(1.0f);
        body.setOutlineColor(Theme::SunsetGlow);
        window.draw(body);

        sf::RectangleShape lower(sf::Vector2f(b.width, b.height * 0.5f - 4.0f));
        lower.setPosition(b.left, b.top + b.height * 0.5f);
        lower.setFillColor(sf::Color(120, 50, 10, 34));
        window.draw(lower);

        Theme::DrawCrispText(window, font, "CREATE PROJECT", 20, b.left + b.width * 0.5f, b.top + b.height * 0.5f, Theme::SunsetDeepDark, sf::Color::Transparent, true, true);
    }

    window.setView(savedView);
}
