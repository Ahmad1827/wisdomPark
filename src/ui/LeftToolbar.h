#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

enum class PanelState {
    Hidden,
    Visible,
    Pinned
};

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
    sf::RectangleShape handleBg;
    sf::Text handleLabel;

    sf::RectangleShape pinBtn;
    sf::Text pinLabel;

    std::vector<ToolItem> tools;
    sf::Font font;
    std::string activeToolId;

    float width;
    float currentX;
    float targetX;
    PanelState state;

public:
    LeftToolbar();
    void init();
    void update(float dt, bool focusMode);
    void updateHover(sf::Vector2f mousePos);
    void draw(sf::RenderWindow& window, bool isAIConfigured);
    std::string handleClick(sf::Vector2f mousePos, bool isAIConfigured);

    float getPanelRightEdge() const;
    std::string getActiveTool() const;
    void setActiveTool(const std::string& id);
};