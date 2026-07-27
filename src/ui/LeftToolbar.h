#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

enum class PanelState { Hidden, Visible, Pinned };

struct ToolItem {
    std::string id;
    sf::RectangleShape rect;
    sf::Text label;
    bool isHovered = false;
    bool isAiTool = false;
};

class LeftToolbar {
public:
    LeftToolbar();
    void init();
    void update(float dt, bool focusMode);
    void updateHover(sf::Vector2f mousePos);
    void draw(sf::RenderWindow& window, bool isAIConfigured, bool hasSelection);
    std::string handleClick(sf::Vector2f mousePos, bool isAIConfigured, bool hasSelection);

    float getPanelRightEdge() const;
    std::string getActiveTool() const;
    void setActiveTool(const std::string& id);

private:
    sf::RectangleShape background;
    sf::RectangleShape handleBg;
    sf::Text handleLabel;

    sf::RectangleShape pinBtn;
    sf::Text pinLabel;

    std::vector<ToolItem> tools;
    std::vector<ToolItem> selectionActions;

    sf::Font font;
    PanelState state;

    float width;
    float currentX;
    float targetX;
    std::string activeToolId;
};