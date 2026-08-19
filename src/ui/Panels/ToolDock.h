#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <functional>

namespace WisdomUI {

    struct ToolDefinition {
        std::string id;
        std::string tooltip;
        sf::IntRect iconRect; // Coordinates in a master icon sprite sheet
        std::function<void()> onSelect;
    };

    class ToolDock {
    public:
        ToolDock();
        void Initialize(const sf::Texture& iconTexture, const sf::Font& font);
        void SetBounds(const sf::FloatRect& bounds);
        void AddTool(const ToolDefinition& tool);
        void SetActiveTool(const std::string& id);

        bool HandleEvent(const sf::Event& event, const sf::RenderWindow& window);
        void Update(float deltaTime, const sf::Vector2f& mousePos);
        void Render(sf::RenderWindow& window);

    private:
        sf::FloatRect m_bounds;
        sf::RectangleShape m_background;
        sf::RectangleShape m_rightBorder;

        const sf::Texture* m_iconTexture{ nullptr };
        const sf::Font* m_font{ nullptr };

        std::vector<ToolDefinition> m_tools;
        std::string m_activeToolId;
        std::string m_hoveredToolId;

        float m_buttonSize = 32.0f;
        float m_spacing = 8.0f;
    };

}