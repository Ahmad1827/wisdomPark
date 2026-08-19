#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <functional>
#include "../UIAnimation.h"

namespace WisdomUI {

    struct RightDockTab {
        std::string id;
        std::string tooltip;
        sf::FloatRect bounds;
        std::function<void()> onToggle;
        bool isToggled{ false };
        float hoverAlpha{ 0.0f };
        float scale{ 1.0f };
    };

    class RightDockTabs {
    public:
        RightDockTabs();
        void Initialize(const sf::Font& font,
            std::function<void()> onToggleLayers,
            std::function<void()> onTogglePalette,
            std::function<void()> onToggleProperties,
            std::function<void()> onToggleAssets,
            std::function<void()> onToggleAudio);

        void SetBounds(const sf::FloatRect& bounds);
        void SetTabState(const std::string& id, bool isToggled);
        bool HandleEvent(const sf::Event& event, const sf::RenderWindow& window);
        void Update(float deltaTime, const sf::Vector2f& mousePos);
        void Render(sf::RenderWindow& window);

    private:
        sf::FloatRect m_bounds;
        sf::Font m_font;
        std::vector<RightDockTab> m_tabs;

        std::string m_hoveredTooltip{ "" };
        sf::Vector2f m_tooltipPos;
        float m_tooltipAlpha{ 0.0f };
    };

}