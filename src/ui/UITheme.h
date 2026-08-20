#pragma once
#include <SFML/Graphics.hpp>
#include "UIAnimation.h"
#include <algorithm>

namespace WisdomUI {

    struct Theme {
        static inline const sf::Color WoodDark = sf::Color(44, 25, 14);
        static inline const sf::Color WoodMedium = sf::Color(74, 44, 25);
        static inline const sf::Color WoodLight = sf::Color(112, 70, 40);
        static inline const sf::Color WoodHighlight = sf::Color(148, 96, 58);

        static inline const sf::Color Parchment = sf::Color(232, 214, 182);
        static inline const sf::Color ParchmentDark = sf::Color(206, 184, 146);
        static inline const sf::Color ParchmentInset = sf::Color(182, 158, 120);

        static inline const sf::Color Brass = sf::Color(184, 134, 48);
        static inline const sf::Color BrassDark = sf::Color(120, 84, 28);
        static inline const sf::Color Gold = sf::Color(248, 204, 72);
        static inline const sf::Color GoldHighlight = sf::Color(255, 238, 140);
        static inline const sf::Color RubyAccent = sf::Color(176, 42, 32);
        static inline const sf::Color RubyHighlight = sf::Color(218, 64, 52);

        static inline const sf::Color TextParchment = sf::Color(48, 30, 16);
        static inline const sf::Color TextParchmentMuted = sf::Color(110, 84, 58);
        static inline const sf::Color TextGold = sf::Color(255, 222, 112);
        static inline const sf::Color TextPrimary = sf::Color(250, 240, 222);
        static inline const sf::Color TextSecondary = sf::Color(214, 186, 146);
        static inline const sf::Color TextMuted = sf::Color(142, 116, 86);
        static inline const sf::Color Transparent = sf::Color(0, 0, 0, 0);

        static inline const sf::Color Background = WoodDark;
        static inline const sf::Color Panel = WoodMedium;
        static inline const sf::Color PanelHover = WoodLight;
        static inline const sf::Color PanelInset = sf::Color(28, 16, 9);
        static inline const sf::Color Border = Brass;
        static inline const sf::Color BorderHighlight = Gold;
        static inline const sf::Color BorderShadow = BrassDark;
        static inline const sf::Color Accent = RubyAccent;
        static inline const sf::Color AccentHover = RubyHighlight;
        static inline const sf::Color AccentGlow = sf::Color(255, 120, 80, 120);

        static inline const float TopBarHeight = 36.0f;
        static inline const float OptionsBarHeight = 32.0f;
        static inline const float ToolDockWidth = 52.0f;
        static inline const float RightDockWidth = 280.0f;
        static inline const float TimelineHeight = 200.0f;
        static inline const float StatusBarHeight = 24.0f;
        static inline const float BorderThickness = 1.0f;

        static void DrawCornerBrackets(sf::RenderWindow& window, sf::FloatRect bounds, float alphaMult = 1.0f) {
            auto drawL = [&](float x, float y, float sx, float sy) {
                sf::RectangleShape h(sf::Vector2f(8.0f * sx, 2.0f * sy));
                h.setPosition(x, y);
                sf::Color g = Gold;
                g.a = static_cast<sf::Uint8>(g.a * alphaMult);
                h.setFillColor(g);
                window.draw(h);

                sf::RectangleShape v(sf::Vector2f(2.0f * sx, 8.0f * sy));
                v.setPosition(x, y);
                v.setFillColor(g);
                window.draw(v);

                sf::RectangleShape rivet(sf::Vector2f(2.0f, 2.0f));
                rivet.setPosition(x + 3.0f * sx, y + 3.0f * sy);
                sf::Color rCol = BrassDark;
                rCol.a = static_cast<sf::Uint8>(rCol.a * alphaMult);
                rivet.setFillColor(rCol);
                window.draw(rivet);
                };

            drawL(bounds.left + 2.0f, bounds.top + 2.0f, 1.0f, 1.0f);
            drawL(bounds.left + bounds.width - 2.0f, bounds.top + 2.0f, -1.0f, 1.0f);
            drawL(bounds.left + 2.0f, bounds.top + bounds.height - 2.0f, 1.0f, -1.0f);
            drawL(bounds.left + bounds.width - 2.0f, bounds.top + bounds.height - 2.0f, -1.0f, -1.0f);
        }

        static void DrawWoodPanel(sf::RenderWindow& window, sf::FloatRect bounds, float alphaMult = 1.0f) {
            sf::RectangleShape shadow(sf::Vector2f(bounds.width + 4.0f, bounds.height + 4.0f));
            shadow.setPosition(bounds.left - 2.0f, bounds.top - 2.0f);
            shadow.setFillColor(sf::Color(12, 7, 4, static_cast<sf::Uint8>(180 * alphaMult)));
            window.draw(shadow);

            sf::RectangleShape outer(sf::Vector2f(bounds.width, bounds.height));
            outer.setPosition(bounds.left, bounds.top);
            sf::Color wBase = WoodMedium;
            wBase.a = static_cast<sf::Uint8>(wBase.a * alphaMult);
            outer.setFillColor(wBase);
            outer.setOutlineThickness(2.0f);
            sf::Color bCol = Brass;
            bCol.a = static_cast<sf::Uint8>(bCol.a * alphaMult);
            outer.setOutlineColor(bCol);
            window.draw(outer);

            sf::RectangleShape bevelTop(sf::Vector2f(bounds.width - 4.0f, 2.0f));
            bevelTop.setPosition(bounds.left + 2.0f, bounds.top + 2.0f);
            sf::Color wLight = WoodLight;
            wLight.a = static_cast<sf::Uint8>(wLight.a * alphaMult);
            bevelTop.setFillColor(wLight);
            window.draw(bevelTop);

            sf::RectangleShape innerFrame(sf::Vector2f(bounds.width - 8.0f, bounds.height - 8.0f));
            innerFrame.setPosition(bounds.left + 4.0f, bounds.top + 4.0f);
            innerFrame.setFillColor(Transparent);
            innerFrame.setOutlineThickness(1.0f);
            sf::Color bDark = BrassDark;
            bDark.a = static_cast<sf::Uint8>(bDark.a * alphaMult);
            innerFrame.setOutlineColor(bDark);
            window.draw(innerFrame);

            DrawCornerBrackets(window, bounds, alphaMult);
        }

        static void DrawParchmentPanel(sf::RenderWindow& window, sf::FloatRect bounds, float alphaMult = 1.0f) {
            DrawWoodPanel(window, bounds, alphaMult);

            sf::FloatRect pBounds(bounds.left + 6.0f, bounds.top + 6.0f, bounds.width - 12.0f, bounds.height - 12.0f);
            sf::RectangleShape pBg(sf::Vector2f(pBounds.width, pBounds.height));
            pBg.setPosition(pBounds.left, pBounds.top);
            sf::Color pCol = Parchment;
            pCol.a = static_cast<sf::Uint8>(pCol.a * alphaMult);
            pBg.setFillColor(pCol);
            pBg.setOutlineThickness(1.0f);
            sf::Color pBorder = ParchmentDark;
            pBorder.a = static_cast<sf::Uint8>(pBorder.a * alphaMult);
            pBg.setOutlineColor(pBorder);
            window.draw(pBg);

            sf::RectangleShape pHeader(sf::Vector2f(pBounds.width, 24.0f));
            pHeader.setPosition(pBounds.left, pBounds.top);
            sf::Color pHCol = ParchmentDark;
            pHCol.a = static_cast<sf::Uint8>(pHCol.a * alphaMult);
            pHeader.setFillColor(pHCol);
            window.draw(pHeader);
        }

        static void DrawFiligreePanel(sf::RenderWindow& window, sf::FloatRect bounds, float alphaMult = 1.0f) {
            DrawWoodPanel(window, bounds, alphaMult);
        }
    };

}