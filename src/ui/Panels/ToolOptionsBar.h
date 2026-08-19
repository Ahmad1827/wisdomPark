#pragma once
#include <SFML/Graphics.hpp>
#include <functional>
#include <string>
#include "../UIAnimation.h"

namespace WisdomUI {

    class ToolOptionsBar {
    public:
        ToolOptionsBar();
        void Initialize(const sf::Font& font);
        void SetBounds(const sf::FloatRect& bounds);
        void SyncState(const std::string& toolName, float size, bool pixelMode, bool pixelPerfect);

        bool HandleEvent(const sf::Event& event, const sf::RenderWindow& window,
            std::function<void(float)> onSizeChange,
            std::function<void()> onTogglePixelPerfect);

        void Update(float deltaTime, const sf::Vector2f& mousePos);
        void Render(sf::RenderWindow& window);

    private:
        sf::FloatRect m_bounds;
        sf::Font m_font;
        std::string m_activeToolName{ "Brush" };
        float m_size{ 4.0f };
        bool m_pixelMode{ false };
        bool m_pixelPerfect{ false };
        bool m_isDraggingSlider{ false };

        sf::FloatRect m_sliderBounds;
        sf::FloatRect m_perfBtnBounds;

        float m_perfHoverAlpha{ 0.0f };
        float m_perfToggleProgress{ 0.0f };
        float m_sliderThumbScale{ 1.0f };
        float m_globalTime{ 0.0f };
    };

}