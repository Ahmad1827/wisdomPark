#include "BottomTimeline.h"

BottomTimeline::BottomTimeline() : scrollOffset(0.0f) {}

void BottomTimeline::init() {
    font.loadFromFile("assets/font.otf");

    background.setSize(sf::Vector2f(1590.f, 150.f));
    background.setPosition(90.f, 1080.f - 150.f);
    background.setFillColor(sf::Color(30, 30, 35, 230));

    auto makeBtn = [&](std::string id, std::string text, float x) {
        TimelineButton btn;
        btn.id = id;
        btn.rect.setSize(sf::Vector2f(60.f, 40.f));
        btn.rect.setPosition(background.getPosition().x + x, background.getPosition().y + 10.f);

        btn.label.setFont(font);
        btn.label.setString(text);
        btn.label.setCharacterSize(14);

        sf::FloatRect tRect = btn.label.getLocalBounds();
        btn.label.setOrigin(tRect.left + tRect.width / 2.0f, tRect.top + tRect.height / 2.0f);
        btn.label.setPosition(btn.rect.getPosition().x + 30.f, btn.rect.getPosition().y + 20.f);

        buttons.push_back(btn);
        };

    makeBtn("play", "Play", 20.f);
    makeBtn("add", "Add", 90.f);
    makeBtn("dup", "Dup", 160.f);
    makeBtn("del", "Del", 230.f);
}

void BottomTimeline::updateHover(sf::Vector2f mousePos) {
    for (auto& btn : buttons) {
        btn.isHovered = btn.rect.getGlobalBounds().contains(mousePos);
    }
}

void BottomTimeline::draw(sf::RenderWindow& window, Timeline& timeline, Canvas& canvas) {
    window.draw(background);

    for (auto& btn : buttons) {
        if (btn.id == "play" && timeline.isPlaying()) {
            btn.rect.setFillColor(sf::Color(0, 122, 204, 255));
        }
        else {
            btn.rect.setFillColor(btn.isHovered ? sf::Color(70, 70, 75, 255) : sf::Color(50, 50, 55, 200));
        }
        btn.label.setFillColor(sf::Color::White);
        window.draw(btn.rect);
        window.draw(btn.label);
    }

    float startX = background.getPosition().x + 20.f;
    float y = background.getPosition().y + 60.f;

    for (size_t i = 0; i < canvas.getFrameCount(); ++i) {
        sf::RectangleShape fRect(sf::Vector2f(50.f, 70.f));
        fRect.setPosition(startX + i * 60.f, y);

        if (i == timeline.getCurrentFrame()) {
            fRect.setFillColor(sf::Color(0, 122, 204));
            fRect.setOutlineThickness(2.f);
            fRect.setOutlineColor(sf::Color::White);
        }
        else {
            fRect.setFillColor(sf::Color(60, 60, 65));
            fRect.setOutlineThickness(1.f);
            fRect.setOutlineColor(sf::Color(20, 20, 20));
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
    for (const auto& btn : buttons) {
        if (btn.rect.getGlobalBounds().contains(mousePos)) {
            return btn.id;
        }
    }
    return "";
}

int BottomTimeline::handleFrameClick(sf::Vector2f mousePos, size_t frameCount) {
    float startX = background.getPosition().x + 20.f;
    float y = background.getPosition().y + 60.f;

    for (size_t i = 0; i < frameCount; ++i) {
        sf::FloatRect bounds(startX + i * 60.f, y, 50.f, 70.f);
        if (bounds.contains(mousePos)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}