#include "BottomTimeline.h"

BottomTimeline::BottomTimeline() : height(140.f), currentY(1080.f), targetY(1080.f), state(BottomPanelState::Hidden) {}

void BottomTimeline::init() {
    font.loadFromFile("assets/font.otf");

    background.setSize(sf::Vector2f(1920.f, height));
    background.setFillColor(sf::Color(15, 15, 18, 220));
    background.setOutlineThickness(1.f);
    background.setOutlineColor(sf::Color(255, 255, 255, 15));

    handleBg.setSize(sf::Vector2f(120.f, 24.f));
    handleBg.setFillColor(sf::Color(30, 30, 35, 200));
    handleBg.setOutlineThickness(1.f);
    handleBg.setOutlineColor(sf::Color(255, 255, 255, 30));

    handleLabel.setFont(font);
    handleLabel.setString("^ TIMELINE");
    handleLabel.setCharacterSize(12);
    handleLabel.setFillColor(sf::Color(200, 200, 200));

    pinBtn.setSize(sf::Vector2f(80.f, 24.f));
    pinBtn.setFillColor(sf::Color(255, 255, 255, 10));

    pinLabel.setFont(font);
    pinLabel.setString("Pin");
    pinLabel.setCharacterSize(12);
    pinLabel.setFillColor(sf::Color(180, 180, 180));

    auto makeBtn = [&](std::string id, std::string text, float x) {
        TimelineButton btn;
        btn.id = id;
        btn.rect.setSize(sf::Vector2f(70.f, 30.f));

        btn.label.setFont(font);
        btn.label.setString(text);
        btn.label.setCharacterSize(12);

        sf::FloatRect tRect = btn.label.getLocalBounds();
        btn.label.setOrigin(tRect.left + tRect.width / 2.0f, tRect.top + tRect.height / 2.0f);

        buttons.push_back(btn);
        };

    makeBtn("play", "Play", 120.f);
    makeBtn("add", "Add", 200.f);
    makeBtn("dup", "Dup", 280.f);
    makeBtn("del", "Del", 360.f);
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

    handleBg.setPosition(1920.f / 2.f - 60.f, currentY - 24.f);
    handleLabel.setPosition(1920.f / 2.f - 30.f, currentY - 20.f);

    if (state == BottomPanelState::Pinned) {
        handleLabel.setString("x");
        pinLabel.setString("Unpin");
        pinLabel.setFillColor(sf::Color(0, 191, 255));
    }
    else {
        handleLabel.setString("^ TIMELINE");
        pinLabel.setString("Pin");
        pinLabel.setFillColor(sf::Color(180, 180, 180));
    }

    pinBtn.setPosition(20.f, currentY + 15.f);
    pinLabel.setPosition(45.f, currentY + 18.f);

    float startX = 120.f;
    for (auto& btn : buttons) {
        btn.rect.setPosition(startX, currentY + 12.f);
        btn.label.setPosition(startX + 35.f, currentY + 26.f);
        startX += 80.f;
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

void BottomTimeline::draw(sf::RenderWindow& window, Timeline& timeline, Canvas& canvas) {
    window.draw(background);

    if (state != BottomPanelState::Pinned) {
        window.draw(handleBg);
        window.draw(handleLabel);
    }

    pinBtn.setFillColor(pinBtn.getGlobalBounds().contains(sf::Vector2f(sf::Mouse::getPosition(window))) ? sf::Color(255, 255, 255, 25) : sf::Color(255, 255, 255, 10));
    window.draw(pinBtn);
    window.draw(pinLabel);

    for (auto& btn : buttons) {
        if (btn.id == "play" && timeline.isPlaying()) {
            btn.rect.setFillColor(sf::Color(0, 122, 204, 180));
        }
        else {
            btn.rect.setFillColor(btn.isHovered ? sf::Color(255, 255, 255, 20) : sf::Color(255, 255, 255, 5));
        }
        btn.label.setFillColor(sf::Color(230, 230, 235));
        window.draw(btn.rect);
        window.draw(btn.label);
    }

    float startX = 20.f;
    float y = currentY + 60.f;

    for (size_t i = 0; i < canvas.getFrameCount(); ++i) {
        sf::RectangleShape fRect(sf::Vector2f(50.f, 60.f));
        fRect.setPosition(startX + i * 60.f, y);

        if (static_cast<int>(i) == timeline.getCurrentFrame()) {
            fRect.setFillColor(sf::Color(0, 122, 204, 150));
            fRect.setOutlineThickness(1.f);
            fRect.setOutlineColor(sf::Color(255, 255, 255, 200));
        }
        else {
            fRect.setFillColor(sf::Color(255, 255, 255, 5));
            fRect.setOutlineThickness(1.f);
            fRect.setOutlineColor(sf::Color(255, 255, 255, 20));
        }
        window.draw(fRect);

        sf::Text fNum;
        fNum.setFont(font);
        fNum.setString(std::to_string(i + 1));
        fNum.setCharacterSize(12);
        fNum.setFillColor(sf::Color::White);
        fNum.setPosition(fRect.getPosition().x + 5.f, fRect.getPosition().y + 5.f);
        window.draw(fNum);
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
    float y = currentY + 60.f;

    for (size_t i = 0; i < frameCount; ++i) {
        sf::FloatRect bounds(startX + i * 60.f, y, 50.f, 60.f);
        if (bounds.contains(mousePos)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

float BottomTimeline::getPanelTopEdge() const { return currentY; }