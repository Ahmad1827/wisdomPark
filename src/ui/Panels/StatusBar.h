#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <functional>
#include "../UIAnimation.h"

namespace WisdomUI {

    class StatusBar {
    public:
        StatusBar();
        void Initialize(const sf::Font& font, std::function<void()> onToggleTimeline);
        void SetBounds(const sf::FloatRect& bounds);
        void UpdateData(sf::Vector2u canvasSize, sf::Vector2f cursorCoords, float zoom, int currentLayer, int currentFrame, bool isTimelineOpen);
        void Update(float deltaTime, const sf::Vector2f& mousePos);
        bool HandleEvent(const sf::Event& event, const sf::RenderWindow& window);
        void Render(sf::RenderWindow& window);

    private:
        sf::FloatRect m_bounds;
        sf::Font m_font;
        sf::Vector2u m_canvasSize{ 0, 0 };
        sf::Vector2f m_cursorCoords{ 0.0f, 0.0f };
        float m_zoom{ 1.0f };
        int m_layer{ 0 };
        int m_frame{ 0 };
        bool m_isTimelineOpen{ false };

        std::function<void()> m_onToggleTimeline;
        sf::FloatRect m_timelineToggleBtn;
        float m_toggleHoverAlpha{ 0.0f };
        float m_globalTime{ 0.0f };
    };

}