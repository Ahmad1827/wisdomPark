#pragma once
#include <SFML/Graphics.hpp>
#include <functional>
#include <string>
#include <vector>

namespace WisdomUI {

    struct ToolItem {
        std::string id;
        std::string tooltip;
        sf::FloatRect bounds;
        float hoverAlpha{ 0.0f };
        float scale{ 1.0f };
        std::function<void()> onSelect;
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
        void RenderTooltip(sf::RenderWindow& window);

    private:
        sf::FloatRect m_bounds;
        sf::Font m_font;
        std::vector<ToolItem> m_tools;
        std::string m_activeToolId{ "brush" };

        float m_selectionSliderY{ 0.0f };
        float m_targetSelectionSliderY{ 0.0f };

        std::string m_hoveredTooltip{ "" };
        sf::Vector2f m_tooltipPos{ 0.0f, 0.0f };
        float m_tooltipAlpha{ 0.0f };
        float m_globalTime{ 0.0f };
    };

}