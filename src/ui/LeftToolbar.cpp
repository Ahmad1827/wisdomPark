#include "LeftToolbar.h"

LeftToolbar::LeftToolbar() : activeToolId("pencil") {}

void LeftToolbar::init() {
    font.loadFromFile("assets/font.otf");

    background.setSize(sf::Vector2f(90.f, 1040.f));
    background.setPosition(0.f, 40.f);
    background.setFillColor(sf::Color(30, 30, 35, 230));

    auto makeBtn = [&](std::string id, std::string text, float y, bool aiTool = false) {
        ToolItem btn;
        btn.id = id;
        btn.isAiTool = aiTool;
        btn.rect.setSize(sf::Vector2f(70.f, 70.f));
        btn.rect.setPosition(10.f, y);

        btn.label.setFont(font);
        btn.label.setString(text);
        btn.label.setCharacterSize(14);

        sf::FloatRect tRect = btn.label.getLocalBounds();
        btn.label.setOrigin(tRect.left + tRect.width / 2.0f, tRect.top + tRect.height / 2.0f);
        btn.label.setPosition(btn.rect.getPosition().x + 35.f, btn.rect.getPosition().y + 35.f);

        tools.push_back(btn);
        };

    float startY = 60.f;
    float gap = 80.f;
    makeBtn("pencil", "Draw", startY);
    makeBtn("eraser", "Erase", startY + gap * 1);
    makeBtn("fill", "Fill", startY + gap * 2);
    makeBtn("select", "Select", startY + gap * 3);
    makeBtn("anim", "Anim", startY + gap * 4);
    makeBtn("layers", "Layers", startY + gap * 5);
    makeBtn("ai_gen", "AI Gen", startY + gap * 6, true);
}

void LeftToolbar::updateHover(sf::Vector2f mousePos) {
    for (auto& tool : tools) {
        tool.isHovered = tool.rect.getGlobalBounds().contains(mousePos);
    }
}

void LeftToolbar::draw(sf::RenderWindow& window, bool isAIConfigured) {
    window.draw(background);
    for (auto& tool : tools) {
        bool disabled = tool.isAiTool && !isAIConfigured;

        if (disabled) {
            tool.rect.setFillColor(sf::Color(40, 40, 45, 200));
            tool.label.setFillColor(sf::Color(100, 100, 100));
        }
        else if (tool.id == activeToolId) {
            tool.rect.setFillColor(sf::Color(0, 122, 204, 255));
            tool.label.setFillColor(sf::Color::White);
        }
        else {
            tool.rect.setFillColor(tool.isHovered ? sf::Color(70, 70, 75, 255) : sf::Color(50, 50, 55, 200));
            tool.label.setFillColor(sf::Color::White);
        }

        window.draw(tool.rect);
        window.draw(tool.label);
    }
}

std::string LeftToolbar::handleClick(sf::Vector2f mousePos, bool isAIConfigured) {
    for (const auto& tool : tools) {
        if (tool.rect.getGlobalBounds().contains(mousePos)) {
            if (tool.isAiTool && !isAIConfigured) return "ai_disabled";
            activeToolId = tool.id;
            return tool.id;
        }
    }
    return "";
}

std::string LeftToolbar::getActiveTool() const { return activeToolId; }
void LeftToolbar::setActiveTool(const std::string& id) { activeToolId = id; }