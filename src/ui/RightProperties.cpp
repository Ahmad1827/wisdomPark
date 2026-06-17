#include "RightProperties.h"
#include <sstream>
#include <iomanip>

RightProperties::RightProperties() {}

void RightProperties::init() {
    font.loadFromFile("assets/font.otf");

    background.setSize(sf::Vector2f(250.f, 1040.f));
    background.setPosition(1920.f - 250.f, 40.f);
    background.setFillColor(sf::Color(30, 30, 35, 230));

    auto createSection = [&](std::string id, std::string title, std::vector<std::pair<std::string, std::string>> btns) {
        PropSection sec;
        sec.id = id;
        sec.isOpen = true;

        sec.headerRect.setSize(sf::Vector2f(250.f, 40.f));
        sec.headerLabel.setFont(font);
        sec.headerLabel.setString(title);
        sec.headerLabel.setCharacterSize(16);
        sec.headerLabel.setFillColor(sf::Color(220, 220, 220));

        for (const auto& pair : btns) {
            PropItem item;
            item.id = pair.first;
            item.rect.setSize(sf::Vector2f(230.f, 35.f));
            item.label.setFont(font);
            item.label.setString(pair.second);
            item.label.setCharacterSize(14);
            sec.items.push_back(item);
        }
        sections.push_back(sec);
        };

    createSection("onion", "Onion Skin", { {"onion_toggle", "Toggle Onion Skin"}, {"onion_op_up", "Opacity +"}, {"onion_op_down", "Opacity -"} });
    createSection("themes", "Themes", { {"theme_all", "All Objects"}, {"theme_struct", "Structures Only"}, {"theme_clutter", "Clutter Only"}, {"theme_custom", "Custom Art"} });
    createSection("gen", "Generation", { {"theme_wfc", "Procedural WFC"}, {"toggle_terrain", "Toggle Terrain"} });
    createSection("fx", "Lighting & FX", { {"toggle_light", "Toggle Lighting"} });

    updateLayout();
}

void RightProperties::updateLayout() {
    float currentY = 40.f;
    for (auto& sec : sections) {
        sec.headerRect.setPosition(1920.f - 250.f, currentY);
        sec.headerLabel.setPosition(1920.f - 240.f, currentY + 10.f);
        currentY += 40.f;

        if (sec.isOpen) {
            for (auto& item : sec.items) {
                item.rect.setPosition(1920.f - 240.f, currentY);
                item.label.setPosition(1920.f - 230.f, currentY + 8.f);
                currentY += 40.f;
            }
        }
    }
}

void RightProperties::updateHover(sf::Vector2f mousePos) {
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
    for (auto& sec : sections) {
        sec.headerRect.setFillColor(sec.isHovered ? sf::Color(55, 55, 60) : sf::Color(45, 45, 50));
        window.draw(sec.headerRect);

        sf::Text arrow = sec.headerLabel;
        arrow.setString(sec.isOpen ? "v" : ">");
        arrow.setPosition(1920.f - 30.f, sec.headerRect.getPosition().y + 10.f);
        window.draw(arrow);
        window.draw(sec.headerLabel);

        if (sec.isOpen) {
            for (auto& item : sec.items) {
                if (item.isActive) {
                    item.rect.setFillColor(sf::Color(0, 122, 204));
                    item.label.setFillColor(sf::Color::White);
                }
                else {
                    item.rect.setFillColor(item.isHovered ? sf::Color(70, 70, 75) : sf::Color(50, 50, 55));
                    item.label.setFillColor(sf::Color(200, 200, 200));
                }
                window.draw(item.rect);
                window.draw(item.label);
            }
        }
    }
}

std::string RightProperties::handleClick(sf::Vector2f mousePos) {
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