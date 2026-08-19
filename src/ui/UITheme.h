#pragma once
#include <SFML/Graphics.hpp>
#include "UIAnimation.h"

namespace WisdomUI {

    struct Theme {
        static inline const sf::Color Background = sf::Color(20, 16, 13, 195);
        static inline const sf::Color Panel = sf::Color(36, 27, 21, 225);
        static inline const sf::Color PanelHover = sf::Color(58, 44, 34, 240);
        static inline const sf::Color PanelInset = sf::Color(16, 12, 9, 235);
        static inline const sf::Color Border = sf::Color(160, 115, 45);
        static inline const sf::Color BorderHighlight = sf::Color(255, 215, 80);
        static inline const sf::Color BorderShadow = sf::Color(12, 9, 7, 200);
        static inline const sf::Color Accent = sf::Color(195, 50, 40);
        static inline const sf::Color AccentHover = sf::Color(235, 75, 55);
        static inline const sf::Color AccentGlow = sf::Color(255, 110, 80, 110);
        static inline const sf::Color Gold = sf::Color(255, 215, 95);
        static inline const sf::Color GoldDim = sf::Color(190, 150, 65);
        static inline const sf::Color TextPrimary = sf::Color(255, 248, 235);
        static inline const sf::Color TextSecondary = sf::Color(210, 180, 140);
        static inline const sf::Color TextMuted = sf::Color(140, 115, 85);
        static inline const sf::Color Transparent = sf::Color(0, 0, 0, 0);

        static inline const float TopBarHeight = 36.0f;
        static inline const float OptionsBarHeight = 32.0f;
        static inline const float ToolDockWidth = 52.0f;
        static inline const float RightDockWidth = 280.0f;
        static inline const float TimelineHeight = 200.0f;
        static inline const float StatusBarHeight = 24.0f;
        static inline const float BorderThickness = 1.0f;

        static void DrawFiligreePanel(sf::RenderWindow& window, sf::FloatRect bounds, float alphaMult = 1.0f) {
            sf::RectangleShape bg(sf::Vector2f(bounds.width, bounds.height));
            bg.setPosition(bounds.left, bounds.top);
            sf::Color bgColor = Panel;
            bgColor.a = static_cast<sf::Uint8>(bgColor.a * alphaMult);
            bg.setFillColor(bgColor);
            window.draw(bg);

            sf::RectangleShape outer(sf::Vector2f(bounds.width, bounds.height));
            outer.setPosition(bounds.left, bounds.top);
            outer.setFillColor(Transparent);
            outer.setOutlineThickness(1.0f);
            sf::Color outCol = Border;
            outCol.a = static_cast<sf::Uint8>(outCol.a * alphaMult);
            outer.setOutlineColor(outCol);
            window.draw(outer);

            sf::RectangleShape inner(sf::Vector2f(bounds.width - 4.0f, bounds.height - 4.0f));
            inner.setPosition(bounds.left + 2.0f, bounds.top + 2.0f);
            inner.setFillColor(Transparent);
            inner.setOutlineThickness(1.0f);
            sf::Color inCol = BorderShadow;
            inCol.a = static_cast<sf::Uint8>(inCol.a * alphaMult);
            inner.setOutlineColor(inCol);
            window.draw(inner);

            auto drawRivet = [&](float rx, float ry) {
                sf::RectangleShape r(sf::Vector2f(2.0f, 2.0f));
                r.setPosition(rx, ry);
                sf::Color goldRivet = BorderHighlight;
                goldRivet.a = static_cast<sf::Uint8>(goldRivet.a * alphaMult);
                r.setFillColor(goldRivet);
                window.draw(r);
                };

            drawRivet(bounds.left + 3.0f, bounds.top + 3.0f);
            drawRivet(bounds.left + bounds.width - 5.0f, bounds.top + 3.0f);
            drawRivet(bounds.left + 3.0f, bounds.top + bounds.height - 5.0f);
            drawRivet(bounds.left + bounds.width - 5.0f, bounds.top + bounds.height - 5.0f);
        }
    };

}