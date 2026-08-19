#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include "../UIAnimation.h"

namespace WisdomUI {

    class StatusBar {
    public:
        StatusBar();
        void Initialize(const sf::Font& font);
        void SetBounds(const sf::FloatRect& bounds);
        void UpdateData(sf::Vector2u canvasSize, sf::Vector2f cursorCoords, float zoom, int currentLayer, int currentFrame);
        void Update(float deltaTime);
        void Render(sf::RenderWindow& window);

    private:
        sf::FloatRect m_bounds;
        sf::Font m_font;
        sf::Vector2u m_canvasSize{ 0, 0 };
        sf::Vector2f m_cursorCoords{ 0.0f, 0.0f };
        float m_zoom{ 1.0f };
        int m_layer{ 0 };
        int m_frame{ 0 };
        float m_globalTime{ 0.0f };
    };

}