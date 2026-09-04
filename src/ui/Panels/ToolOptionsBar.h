#pragma once
#include <SFML/Graphics.hpp>
#include <functional>
#include <string>
#include <vector>
#include "../UIAnimation.h"

namespace WisdomUI {

    struct SelectionToolButton {
        std::string id;
        std::string label;
        sf::FloatRect bounds;
        float hoverAlpha{ 0.0f };
    };

    class ToolOptionsBar {
    public:
        ToolOptionsBar();
        void Initialize(const sf::Font& font);
        void SetBounds(const sf::FloatRect& bounds);
        void SyncState(const std::string& toolName, float size, bool pixelMode, bool pixelPerfect);

        void SetOutlineColor(sf::Color color) { m_outlineColor = color; }
        sf::Color GetOutlineColor() const { return m_outlineColor; }

        bool HandleEvent(const sf::Event& event, const sf::RenderWindow& window,
            std::function<void(float)> onSizeChange,
            std::function<void()> onTogglePixelPerfect,
            std::function<void(const std::string&)> onSelectAction = nullptr,
            std::function<void()> onMakeOutline = nullptr,
            std::function<void()> onPickOutlineColor = nullptr);

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
        sf::FloatRect m_outlineBtnBounds;
        sf::FloatRect m_outlineColorBoxBounds;

        sf::Color m_outlineColor{ sf::Color::Black };

        std::vector<SelectionToolButton> m_selectionButtons;

        float m_perfHoverAlpha{ 0.0f };
        float m_perfToggleProgress{ 0.0f };
        float m_outlineHoverAlpha{ 0.0f };
        float m_outlineColorBoxHoverAlpha{ 0.0f };
        float m_sliderThumbScale{ 1.0f };
        float m_globalTime{ 0.0f };

        void updateSelectionButtonLayout();
    };

}