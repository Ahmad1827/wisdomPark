#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <functional>

namespace WisdomUI {

    struct ToolItem {
        std::string id;
        std::string tooltip;
        sf::FloatRect bounds;
        std::function<void()> onSelect;
        bool isHovered{ false };
    };

    class ToolDock {
    public:
        ToolDock();
        void Initialize(const sf::Font& font);
        void SetBounds(const sf::FloatRect& bounds);
        void AddTool(const std::string& id, const std::string& tooltip, std::function<void()> onSelect);
        void SetActiveTool(const std::string& id);

        bool HandleEvent(const sf::Event& event, const sf::RenderWindow& window);
        void Update(float deltaTime, const sf::Vector2f& mousePos);
        void Render(sf::RenderWindow& window);

    private:
        sf::FloatRect m_bounds;
        sf::Font m_font;
        std::vector<ToolItem> m_tools;
        std::string m_activeToolId{ "brush" };
        std::string m_hoveredTooltip{ "" };
        sf::Vector2f m_tooltipPos;
    };

}