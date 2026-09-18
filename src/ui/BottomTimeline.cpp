#include "BottomTimeline.h"
#include <algorithm>
#include <cmath>

BottomTimeline::BottomTimeline()
    : height(154.f), currentY(1080.f), targetY(1080.f), state(BottomPanelState::Hidden),
    onionEnabled(true), onionPrev(1), onionNext(1) {}

void BottomTimeline::init() {
    font.loadFromFile("assets/font.otf");

    background.setSize(sf::Vector2f(1920.f, height));
    background.setFillColor(sf::Color(15, 15, 18, 235));
    background.setOutlineThickness(1.f);
    background.setOutlineColor(sf::Color(255, 255, 255, 25));

    handleBg.setSize(sf::Vector2f(140.f, 28.f));
    handleBg.setFillColor(sf::Color(30, 30, 38, 220));
    handleBg.setOutlineThickness(1.f);
    handleBg.setOutlineColor(sf::Color(255, 255, 255, 40));

    handleLabel.setFont(font);
    handleLabel.setString("^ TIMELINE");
    handleLabel.setCharacterSize(13);
    handleLabel.setFillColor(sf::Color(220, 220, 220));

    pinBtn.setSize(sf::Vector2f(90.f, 28.f));
    pinBtn.setFillColor(sf::Color(255, 255, 255, 12));

    pinLabel.setFont(font);
    pinLabel.setString("Pin");
    pinLabel.setCharacterSize(13);
    pinLabel.setFillColor(sf::Color(190, 190, 190));

    auto makeBtn = [&](std::string id, std::string text, float w) {
        TimelineButton btn;
        btn.id = id;
        btn.rect.setSize(sf::Vector2f(w, 34.f));

        btn.label.setFont(font);
        btn.label.setString(text);
        btn.label.setCharacterSize(14);

        sf::FloatRect tRect = btn.label.getLocalBounds();
        btn.label.setOrigin(tRect.left + tRect.width / 2.0f, tRect.top + tRect.height / 2.0f);

        buttons.push_back(btn);
        };

    makeBtn("play", "Play", 74.f);
    makeBtn("add", "+ Add", 74.f);
    makeBtn("dup", "Dup", 68.f);
    makeBtn("del", "Del", 68.f);
    makeBtn("export", "Export", 82.f);

    makeBtn("onion_toggle", "Onion: ON", 100.f);
    makeBtn("onion_prev", "Prev: 1", 80.f);
    makeBtn("onion_next", "Next: 1", 80.f);
}

void BottomTimeline::syncOnionState(bool enabled, int prevCount, int nextCount) {
    onionEnabled = enabled;
    onionPrev = prevCount;
    onionNext = nextCount;
}

void BottomTimeline::update(float dt, bool focusMode) {
    if (focusMode) {
        targetY = 1080.f;
    }
    else {
        if (state == BottomPanelState::Pinned || state == BottomPanelState::Visible) {
            targetY = 1080.f - height;
        }
        else {
            targetY = 1080.f;
        }
    }

    currentY += (targetY - currentY) * 15.0f * dt;

    background.setPosition(0.f, currentY);

    handleBg.setPosition(1920.f / 2.f - 70.f, currentY - 28.f);
    handleLabel.setPosition(1920.f / 2.f - 38.f, currentY - 22.f);

    if (state == BottomPanelState::Pinned) {
        handleLabel.setString("x CLOSE");
        pinLabel.setString("Unpin");
        pinLabel.setFillColor(sf::Color(0, 191, 255));
    }
    else {
        handleLabel.setString("^ TIMELINE");
        pinLabel.setString("Pin");
        pinLabel.setFillColor(sf::Color(190, 190, 190));
    }

    pinBtn.setPosition(20.f, currentY + 12.f);
    pinLabel.setPosition(50.f, currentY + 17.f);

    float startX = 125.f;
    float rightX = 1920.f - 300.f;

    for (auto& btn : buttons) {
        if (btn.id == "onion_toggle") {
            btn.rect.setPosition(rightX, currentY + 10.f);
            btn.label.setPosition(rightX + 50.f, currentY + 27.f);
        }
        else if (btn.id == "onion_prev") {
            btn.rect.setPosition(rightX + 106.f, currentY + 10.f);
            btn.label.setPosition(rightX + 106.f + 40.f, currentY + 27.f);
        }
        else if (btn.id == "onion_next") {
            btn.rect.setPosition(rightX + 192.f, currentY + 10.f);
            btn.label.setPosition(rightX + 192.f + 40.f, currentY + 27.f);
        }
        else {
            btn.rect.setPosition(startX, currentY + 10.f);
            btn.label.setPosition(startX + btn.rect.getSize().x / 2.f, currentY + 27.f);
            startX += btn.rect.getSize().x + 8.f;
        }
    }
}

void BottomTimeline::updateHover(sf::Vector2f mousePos) {
    bool inPanel = background.getGlobalBounds().contains(mousePos);
    bool inHandle = handleBg.getGlobalBounds().contains(mousePos);

    if (state == BottomPanelState::Hidden && inHandle) {
        state = BottomPanelState::Visible;
    }
    else if (state == BottomPanelState::Visible && !inPanel && !inHandle) {
        state = BottomPanelState::Hidden;
    }

    for (auto& btn : buttons) {
        btn.isHovered = btn.rect.getGlobalBounds().contains(mousePos);
    }
}

void BottomTimeline::drawTooltip(sf::RenderWindow& window, const std::string& text, sf::Vector2f pos) {
    sf::Text tipText(text, font, 13);
    sf::FloatRect bounds = tipText.getLocalBounds();

    float padX = 10.f;
    float padY = 6.f;
    float w = bounds.width + padX * 2.f;
    float h = bounds.height + padY * 2.f + 4.f;

    float x = pos.x - w / 2.f;
    float y = pos.y - h - 12.f;

    if (x < 10.f) x = 10.f;
    if (x + w > 1910.f) x = 1910.f - w;

    sf::RectangleShape bg(sf::Vector2f(w, h));
    bg.setPosition(x, y);
    bg.setFillColor(sf::Color(18, 12, 24, 250));
    bg.setOutlineThickness(1.5f);
    bg.setOutlineColor(sf::Color(255, 215, 60));

    tipText.setPosition(x + padX, y + padY - 2.f);
    tipText.setFillColor(sf::Color::White);

    window.draw(bg);
    window.draw(tipText);
}

void BottomTimeline::draw(sf::RenderWindow& window, Timeline& timeline, Canvas& canvas) {
    window.draw(background);

    sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    std::string hoveredTooltip = "";

    if (state != BottomPanelState::Pinned) {
        window.draw(handleBg);
        window.draw(handleLabel);
    }

    bool hovPin = pinBtn.getGlobalBounds().contains(mousePos);
    if (hovPin) hoveredTooltip = (state == BottomPanelState::Pinned) ? "Unpin timeline panel" : "Pin timeline panel open";
    pinBtn.setFillColor(hovPin ? sf::Color(255, 255, 255, 30) : sf::Color(255, 255, 255, 12));
    window.draw(pinBtn);
    window.draw(pinLabel);

    for (auto& btn : buttons) {
        if (btn.isHovered) {
            if (btn.id == "play") hoveredTooltip = timeline.isPlaying() ? "Pause animation preview (Space)" : "Play animation sequence (Space)";
            else if (btn.id == "add") hoveredTooltip = "Add new blank frame after current frame";
            else if (btn.id == "dup") hoveredTooltip = "Duplicate active frame with all layer contents";
            else if (btn.id == "del") hoveredTooltip = "Delete active frame from timeline";
            else if (btn.id == "export") hoveredTooltip = "Open Export modal to export sequence or spritesheet";
            else if (btn.id == "onion_toggle") hoveredTooltip = "Toggle onion skin transparency ghosting";
            else if (btn.id == "onion_prev") hoveredTooltip = "Adjust number of preceding onion frames";
            else if (btn.id == "onion_next") hoveredTooltip = "Adjust number of succeeding onion frames";
        }

        if (btn.id == "play" && timeline.isPlaying()) {
            btn.rect.setFillColor(sf::Color(0, 122, 204, 200));
        }
        else if (btn.id == "onion_toggle") {
            btn.label.setString(onionEnabled ? "Onion: ON" : "Onion: OFF");
            btn.rect.setFillColor(onionEnabled ? sf::Color(0, 122, 204, 200) : (btn.isHovered ? sf::Color(255, 255, 255, 25) : sf::Color(255, 255, 255, 8)));
            sf::FloatRect tRect = btn.label.getLocalBounds();
            btn.label.setOrigin(tRect.left + tRect.width / 2.0f, tRect.top + tRect.height / 2.0f);
        }
        else if (btn.id == "onion_prev") {
            btn.label.setString("Prev: " + std::to_string(onionPrev));
            btn.rect.setFillColor(btn.isHovered ? sf::Color(255, 255, 255, 25) : sf::Color(255, 255, 255, 8));
            sf::FloatRect tRect = btn.label.getLocalBounds();
            btn.label.setOrigin(tRect.left + tRect.width / 2.0f, tRect.top + tRect.height / 2.0f);
        }
        else if (btn.id == "onion_next") {
            btn.label.setString("Next: " + std::to_string(onionNext));
            btn.rect.setFillColor(btn.isHovered ? sf::Color(255, 255, 255, 25) : sf::Color(255, 255, 255, 8));
            sf::FloatRect tRect = btn.label.getLocalBounds();
            btn.label.setOrigin(tRect.left + tRect.width / 2.0f, tRect.top + tRect.height / 2.0f);
        }
        else if (btn.id == "export") {
            btn.rect.setFillColor(btn.isHovered ? sf::Color(0, 160, 255, 220) : sf::Color(0, 122, 204, 200));
        }
        else {
            btn.rect.setFillColor(btn.isHovered ? sf::Color(255, 255, 255, 25) : sf::Color(255, 255, 255, 8));
        }
        btn.label.setFillColor(sf::Color(235, 235, 240));
        window.draw(btn.rect);
        window.draw(btn.label);
    }

    float startX = 20.f;
    float y = currentY + 64.f;

    for (size_t i = 0; i < canvas.getFrameCount(); ++i) {
        sf::FloatRect frameBounds(startX + i * 66.f, y, 56.f, 68.f);
        if (frameBounds.contains(mousePos)) {
            hoveredTooltip = "Frame " + std::to_string(i + 1) + " (Click to select)";
        }

        sf::RectangleShape fRect(sf::Vector2f(frameBounds.width, frameBounds.height));
        fRect.setPosition(frameBounds.left, frameBounds.top);

        if (static_cast<int>(i) == timeline.getCurrentFrame()) {
            fRect.setFillColor(sf::Color(0, 122, 204, 180));
            fRect.setOutlineThickness(1.5f);
            fRect.setOutlineColor(sf::Color(255, 255, 255, 240));
        }
        else {
            fRect.setFillColor(frameBounds.contains(mousePos) ? sf::Color(255, 255, 255, 16) : sf::Color(255, 255, 255, 6));
            fRect.setOutlineThickness(1.f);
            fRect.setOutlineColor(sf::Color(255, 255, 255, 24));
        }
        window.draw(fRect);

        sf::Text fNum;
        fNum.setFont(font);
        fNum.setString(std::to_string(i + 1));
        fNum.setCharacterSize(14);
        fNum.setFillColor(sf::Color::White);
        fNum.setPosition(fRect.getPosition().x + 8.f, fRect.getPosition().y + 6.f);
        window.draw(fNum);
    }

    if (!hoveredTooltip.empty()) {
        drawTooltip(window, hoveredTooltip, mousePos);
    }
}

std::string BottomTimeline::handleClick(sf::Vector2f mousePos) {
    if (pinBtn.getGlobalBounds().contains(mousePos)) {
        state = (state == BottomPanelState::Pinned) ? BottomPanelState::Visible : BottomPanelState::Pinned;
        return "pin_toggle";
    }

    if (state == BottomPanelState::Hidden && handleBg.getGlobalBounds().contains(mousePos)) {
        state = BottomPanelState::Pinned;
        return "handle_click";
    }

    for (const auto& btn : buttons) {
        if (btn.rect.getGlobalBounds().contains(mousePos)) {
            return btn.id;
        }
    }
    return "";
}

int BottomTimeline::handleFrameClick(sf::Vector2f mousePos, size_t frameCount) {
    float startX = 20.f;
    float y = currentY + 64.f;

    for (size_t i = 0; i < frameCount; ++i) {
        sf::FloatRect bounds(startX + i * 66.f, y, 56.f, 68.f);
        if (bounds.contains(mousePos)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

float BottomTimeline::getPanelTopEdge() const { return currentY; }