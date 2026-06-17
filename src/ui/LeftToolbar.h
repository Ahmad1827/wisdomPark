#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

struct ToolItem {
    sf::RectangleShape rect;
    sf::Text label;
    std::string id;
    bool isHovered = false;
    bool isAiTool = false;
};

class LeftToolbar {
private:
    sf::RectangleShape background;
    std::vector<ToolItem> tools;
    sf::Font font;
    std::string activeToolId;

public:
    LeftToolbar();
    void init();
    void updateHover(sf::Vector2f mousePos);
    void draw(sf::RenderWindow& window, bool isAIConfigured);
    std::string handleClick(sf::Vector2f mousePos, bool isAIConfigured);
    std::string getActiveTool() const;
    void setActiveTool(const std::string& id);
};