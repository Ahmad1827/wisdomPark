#include "RightProperties.h"
#include <sstream>

RightProperties::RightProperties() : width(260.f), currentX(1920.f), targetX(1920.f), state(RightPanelState::Hidden) {}

void RightProperties::init() {
    font.loadFromFile("assets/font.otf");

    background.setSize(sf::Vector2f(width, 1080.f));
    background.setFillColor(sf::Color(15, 15, 18, 220));
    background.setOutlineThickness(1.f);
    background.setOutlineColor(sf::Color(255, 255, 255, 15));

    handleBg.setSize(sf::Vector2f(24.f, 80.f));
    handleBg.setFillColor(sf::Color(30, 30, 35, 200));
    handleBg.setOutlineThickness(1.f);
    handleBg.setOutlineColor(sf::Color(255, 255, 255, 30));

    handleLabel.setFont(font);
    handleLabel.setString("<");
    handleLabel.setCharacterSize(16);
    handleLabel.setFillColor(sf::Color(200, 200, 200));

    pinBtn.setSize(sf::Vector2f(width - 40.f, 24.f));
    pinBtn.setFillColor(sf::Color(255, 255, 255, 10));

    pinLabel.setFont(font);
    pinLabel.setString("Pin Panel");
    pinLabel.setCharacterSize(12);
    pinLabel.setFillColor(sf::Color(180, 180, 180));

    auto createSection = [&](std::string id, std::string title, std::vector<std::pair<std::string, std::string>> btns) {
        PropSection sec;
        sec.id = id;
        sec.isOpen = true;

        sec.headerRect.setSize(sf::Vector2f(width, 35.f));
        sec.headerLabel.setFont(font);
        sec.headerLabel.setString(title);
        sec.headerLabel.setCharacterSize(14);
        sec.headerLabel.setFillColor(sf::Color(230, 230, 235));

        for (const auto& pair : btns) {
            PropItem item;
            item.id = pair.first;
            item.rect.setSize(sf::Vector2f(width - 40.f, 30.f));
            item.label.setFont(font);
            item.label.setString(pair.second);
            item.label.setCharacterSize(12);
            sec.items.push_back(item);
        }
        sections.push_back(sec);
        };

    createSection("onion", "ONION SKIN", { {"onion_toggle", "Toggle Onion Skin"}, {"onion_op_up", "Opacity +"}, {"onion_op_down", "Opacity -"} });
    createSection("themes", "THEMES", { {"theme_all", "All Objects"}, {"theme_struct", "Structures Only"}, {"theme_clutter", "Clutter Only"}, {"theme_custom", "Custom Art"} });
    createSection("gen", "GENERATION", { {"theme_wfc", "Procedural WFC"}, {"toggle_terrain", "Toggle Terrain"} });
    createSection("fx", "LIGHTING & FX", { {"toggle_light", "Toggle Lighting"} });

    updateLayout();
}

void RightProperties::update(float dt, bool focusMode) {
    if (focusMode) {
        targetX = 1920.f;
    }
    else {
        if (state == RightPanelState::Pinned || state == RightPanelState::Visible) {
            targetX = 1920.f - width;
        }
        else {
            targetX = 1920.f;
        }
    }

    currentX += (targetX - currentX) * 15.0f * dt;

    background.setPosition(currentX, 0.f);

    handleBg.setPosition(currentX - 24.f, 50.f);
    handleLabel.setPosition(currentX - 18.f, 80.f);

    if (state == RightPanelState::Pinned) {
        handleLabel.setString("x");
        pinLabel.setString("Unpin Panel");
        pinLabel.setFillColor(sf::Color(0, 191, 255));
    }
    else {
        handleLabel.setString("<");
        pinLabel.setString("Pin Panel");
        pinLabel.setFillColor(sf::Color(180, 180, 180));
    }

    pinBtn.setPosition(currentX + 20.f, 20.f);
    pinLabel.setPosition(currentX + (width / 2.f) - 30.f, 24.f);

    updateLayout();
}

void RightProperties::updateLayout() {
    float startY = 70.f;
    for (auto& sec : sections) {
        sec.headerRect.setPosition(currentX, startY);
        sec.headerLabel.setPosition(currentX + 20.f, startY + 10.f);
        startY += 35.f;

        if (sec.isOpen) {
            for (auto& item : sec.items) {
                item.rect.setPosition(currentX + 20.f, startY);
                item.label.setPosition(currentX + 30.f, startY + 8.f);
                startY += 35.f;
            }
            startY += 10.f;
        }
    }
}

void RightProperties::updateHover(sf::Vector2f mousePos) {
    bool inPanel = background.getGlobalBounds().contains(mousePos);
    bool inHandle = handleBg.getGlobalBounds().contains(mousePos);

    if (state == RightPanelState::Hidden && inHandle) {
        state = RightPanelState::Visible;
    }
    else if (state == RightPanelState::Visible && !inPanel && !inHandle) {
        state = RightPanelState::Hidden;
    }

    for (auto& sec : sections) {
        sec.isHovered = sec.headerRect.getGlobalBounds().contains(mousePos);
        if (sec.isOpen) {
            for (auto& item : sec.items) {
                item.isHovered = item.rect.getGlobalBounds().contains(mousePos);
            }
        }
    }
}

void RightProperties::draw(sf::RenderWindow& window) {
    window.draw(background);

    if (state != RightPanelState::Pinned) {
        window.draw(handleBg);
        window.draw(handleLabel);
    }

    pinBtn.setFillColor(pinBtn.getGlobalBounds().contains(sf::Vector2f(sf::Mouse::getPosition(window))) ? sf::Color(255, 255, 255, 25) : sf::Color(255, 255, 255, 10));
    window.draw(pinBtn);
    window.draw(pinLabel);

    for (auto& sec : sections) {
        sec.headerRect.setFillColor(sec.isHovered ? sf::Color(255, 255, 255, 10) : sf::Color::Transparent);
        window.draw(sec.headerRect);

        sf::Text arrow = sec.headerLabel;
        arrow.setString(sec.isOpen ? "v" : "<");
        arrow.setPosition(currentX + width - 30.f, sec.headerRect.getPosition().y + 10.f);
        window.draw(arrow);
        window.draw(sec.headerLabel);

        if (sec.isOpen) {
            for (auto& item : sec.items) {
                if (item.isActive) {
                    item.rect.setFillColor(sf::Color(0, 122, 204, 180));
                    item.label.setFillColor(sf::Color::White);
                }
                else {
                    item.rect.setFillColor(item.isHovered ? sf::Color(255, 255, 255, 20) : sf::Color(255, 255, 255, 5));
                    item.label.setFillColor(sf::Color(220, 220, 225));
                }
                window.draw(item.rect);
                window.draw(item.label);
            }
        }
    }
}

std::string RightProperties::handleClick(sf::Vector2f mousePos) {
    if (pinBtn.getGlobalBounds().contains(mousePos)) {
        state = (state == RightPanelState::Pinned) ? RightPanelState::Visible : RightPanelState::Pinned;
        return "pin_toggle";
    }

    if (state == RightPanelState::Hidden && handleBg.getGlobalBounds().contains(mousePos)) {
        state = RightPanelState::Pinned;
        return "handle_click";
    }

    for (auto& sec : sections) {
        if (sec.headerRect.getGlobalBounds().contains(mousePos)) {
            sec.isOpen = !sec.isOpen;
            updateLayout();
            return "section_toggle";
        }
        if (sec.isOpen) {
            for (auto& item : sec.items) {
                if (item.rect.getGlobalBounds().contains(mousePos)) {
                    return item.id;
                }
            }
        }
    }
    return "";
}

void RightProperties::syncState(const std::string& theme, bool lighting, bool terrain, bool onion, float onionOpacity) {
    for (auto& sec : sections) {
        for (auto& item : sec.items) {
            item.isActive = false;
            if (item.id == "theme_all" && theme == "all") item.isActive = true;
            if (item.id == "theme_struct" && theme == "structure") item.isActive = true;
            if (item.id == "theme_clutter" && theme == "clutter") item.isActive = true;
            if (item.id == "theme_custom" && theme == "custom") item.isActive = true;
            if (item.id == "theme_wfc" && theme == "wfc") item.isActive = true;
            if (item.id == "toggle_light" && lighting) item.isActive = true;
            if (item.id == "toggle_terrain" && terrain) item.isActive = true;
            if (item.id == "onion_toggle" && onion) item.isActive = true;

            if (item.id == "onion_toggle") {
                std::stringstream ss;
                ss << "Toggle Onion (" << static_cast<int>((onionOpacity / 255.f) * 100) << "%)";
                item.label.setString(ss.str());
            }
        }
    }
}

float RightProperties::getCurrentX() const { return currentX; }
void RightProperties::forceClose() { if (state != RightPanelState::Pinned) state = RightPanelState::Hidden; }
bool RightProperties::isHovered() const { return state == RightPanelState::Visible; }
bool RightProperties::isPanelPinned() const { return state == RightPanelState::Pinned; }