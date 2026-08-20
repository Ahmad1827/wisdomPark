#pragma once
#include <SFML/Graphics.hpp>
#include "UIAnimation.h"
#include <algorithm>
#include <cmath>

namespace WisdomUI {

    struct Theme {
        static inline const sf::Color SunsetDeepDark = sf::Color(20, 10, 28);
        static inline const sf::Color SunsetSkyTop = sf::Color(34, 16, 46);
        static inline const sf::Color SunsetSkyMid = sf::Color(64, 26, 72);
        static inline const sf::Color SunsetPlum = sf::Color(96, 38, 92);
        static inline const sf::Color SunsetViolet = sf::Color(142, 62, 140);
        static inline const sf::Color SunsetCoralDark = sf::Color(168, 48, 72);
        static inline const sf::Color SunsetCoral = sf::Color(230, 82, 98);
        static inline const sf::Color SunsetPeach = sf::Color(255, 154, 118);
        static inline const sf::Color SunsetAmber = sf::Color(248, 190, 68);
        static inline const sf::Color SunsetGold = sf::Color(255, 226, 110);
        static inline const sf::Color SunsetGlow = sf::Color(255, 246, 175);

        static inline const sf::Color TextPrimary = sf::Color(255, 245, 235);
        static inline const sf::Color TextSecondary = sf::Color(238, 192, 198);
        static inline const sf::Color TextGold = sf::Color(255, 226, 110);
        static inline const sf::Color TextPeach = sf::Color(255, 174, 140);
        static inline const sf::Color TextMuted = sf::Color(160, 118, 152);
        static inline const sf::Color Transparent = sf::Color(0, 0, 0, 0);

        static inline const sf::Color Background = SunsetDeepDark;
        static inline const sf::Color Panel = SunsetSkyTop;
        static inline const sf::Color PanelHover = SunsetSkyMid;
        static inline const sf::Color PanelInset = SunsetDeepDark;
        static inline const sf::Color Border = SunsetCoralDark;
        static inline const sf::Color BorderHighlight = SunsetAmber;
        static inline const sf::Color BorderShadow = SunsetDeepDark;
        static inline const sf::Color Accent = SunsetCoral;
        static inline const sf::Color AccentHover = SunsetPeach;
        static inline const sf::Color AccentGlow = sf::Color(255, 154, 118, 90);

        static inline const sf::Color WoodDeepShadow = SunsetDeepDark;
        static inline const sf::Color WoodDark = SunsetSkyTop;
        static inline const sf::Color WoodMedium = SunsetSkyMid;
        static inline const sf::Color WoodLight = SunsetPlum;
        static inline const sf::Color WoodHighlight = SunsetViolet;
        static inline const sf::Color Parchment = sf::Color(245, 225, 215);
        static inline const sf::Color ParchmentDark = sf::Color(215, 185, 180);
        static inline const sf::Color ParchmentShadow = sf::Color(170, 130, 140);
        static inline const sf::Color ParchmentInset = sf::Color(195, 155, 165);
        static inline const sf::Color BrassDark = SunsetCoralDark;
        static inline const sf::Color Brass = SunsetCoral;
        static inline const sf::Color Gold = SunsetAmber;
        static inline const sf::Color GoldHighlight = SunsetGold;
        static inline const sf::Color RubyDark = SunsetCoralDark;
        static inline const sf::Color RubyMuted = SunsetCoralDark;
        static inline const sf::Color RubyAccent = SunsetCoral;
        static inline const sf::Color RubyHighlight = SunsetPeach;
        static inline const sf::Color TextParchment = sf::Color(36, 14, 32);
        static inline const sf::Color TextParchmentMuted = sf::Color(110, 60, 80);

        static inline const float TopBarHeight = 36.0f;
        static inline const float OptionsBarHeight = 32.0f;
        static inline const float ToolDockWidth = 52.0f;
        static inline const float RightDockWidth = 300.0f;
        static inline const float TimelineHeight = 200.0f;
        static inline const float StatusBarHeight = 24.0f;
        static inline const float BorderThickness = 1.0f;

        static void DrawCrispText(sf::RenderWindow& window, const sf::Font& font, const std::string& str, unsigned int size, float x, float y, sf::Color color, sf::Color shadowColor = sf::Color::Transparent, bool centerH = false, bool centerV = false) {
            sf::Text txt(str, font, size);
            sf::FloatRect tb = txt.getLocalBounds();

            float drawX = std::floor(x);
            float drawY = std::floor(y);

            if (centerH) drawX = std::floor(x - (tb.left + tb.width / 2.0f));
            if (centerV) drawY = std::floor(y - (tb.top + tb.height / 2.0f));

            if (shadowColor != sf::Color::Transparent) {
                txt.setPosition(drawX + 1.0f, drawY + 1.0f);
                txt.setFillColor(shadowColor);
                window.draw(txt);
            }

            txt.setPosition(drawX, drawY);
            txt.setFillColor(color);
            window.draw(txt);
        }

        static void DrawSunsetPanel(sf::RenderWindow& window, sf::FloatRect bounds, float alphaMult = 1.0f) {
            float x = std::floor(bounds.left);
            float y = std::floor(bounds.top);
            float w = std::floor(bounds.width);
            float h = std::floor(bounds.height);
            float c = 6.0f;

            sf::ConvexShape shadow(8);
            shadow.setPoint(0, sf::Vector2f(x - 2.0f + c, y - 2.0f));
            shadow.setPoint(1, sf::Vector2f(x + w + 2.0f - c, y - 2.0f));
            shadow.setPoint(2, sf::Vector2f(x + w + 2.0f, y - 2.0f + c));
            shadow.setPoint(3, sf::Vector2f(x + w + 2.0f, y + h + 2.0f - c));
            shadow.setPoint(4, sf::Vector2f(x + w + 2.0f - c, y + h + 2.0f));
            shadow.setPoint(5, sf::Vector2f(x - 2.0f + c, y + h + 2.0f));
            shadow.setPoint(6, sf::Vector2f(x - 2.0f, y + h + 2.0f - c));
            shadow.setPoint(7, sf::Vector2f(x - 2.0f, y - 2.0f + c));
            shadow.setFillColor(sf::Color(10, 4, 16, static_cast<sf::Uint8>(210 * alphaMult)));
            window.draw(shadow);

            sf::ConvexShape base(8);
            base.setPoint(0, sf::Vector2f(x + c, y));
            base.setPoint(1, sf::Vector2f(x + w - c, y));
            base.setPoint(2, sf::Vector2f(x + w, y + c));
            base.setPoint(3, sf::Vector2f(x + w, y + h - c));
            base.setPoint(4, sf::Vector2f(x + w - c, y + h));
            base.setPoint(5, sf::Vector2f(x + c, y + h));
            base.setPoint(6, sf::Vector2f(x, y + h - c));
            base.setPoint(7, sf::Vector2f(x, y + c));

            sf::Color bgCol = SunsetSkyTop;
            bgCol.a = static_cast<sf::Uint8>(248 * alphaMult);
            base.setFillColor(bgCol);
            base.setOutlineThickness(1.5f);
            sf::Color borderCol = SunsetCoralDark;
            borderCol.a = static_cast<sf::Uint8>(255 * alphaMult);
            base.setOutlineColor(borderCol);
            window.draw(base);

            sf::RectangleShape topEdge(sf::Vector2f(w - c * 2.0f, 1.0f));
            topEdge.setPosition(x + c, y + 1.0f);
            sf::Color glowCol = SunsetPeach;
            glowCol.a = static_cast<sf::Uint8>(180 * alphaMult);
            topEdge.setFillColor(glowCol);
            window.draw(topEdge);

            auto drawPixelDiamond = [&](float px, float py) {
                sf::ConvexShape d(4);
                d.setPoint(0, sf::Vector2f(0.0f, -2.5f));
                d.setPoint(1, sf::Vector2f(2.5f, 0.0f));
                d.setPoint(2, sf::Vector2f(0.0f, 2.5f));
                d.setPoint(3, sf::Vector2f(-2.5f, 0.0f));
                d.setPosition(std::floor(px), std::floor(py));
                sf::Color dc = SunsetAmber;
                dc.a = static_cast<sf::Uint8>(240 * alphaMult);
                d.setFillColor(dc);
                window.draw(d);
                };

            drawPixelDiamond(x + c, y + c);
            drawPixelDiamond(x + w - c, y + c);
            drawPixelDiamond(x + c, y + h - c);
            drawPixelDiamond(x + w - c, y + h - c);
        }

        static void DrawSunsetButton(sf::RenderWindow& window, sf::FloatRect bounds, const std::string& label, const sf::Font& font, unsigned int charSize, bool active, bool hovered, bool isRuby = false, float scale = 1.0f) {
            float x = std::floor(bounds.left);
            float y = std::floor(bounds.top);
            float w = std::floor(bounds.width);
            float h = std::floor(bounds.height);
            float c = 4.0f;

            sf::ConvexShape btn(8);
            btn.setPoint(0, sf::Vector2f(x + c, y));
            btn.setPoint(1, sf::Vector2f(x + w - c, y));
            btn.setPoint(2, sf::Vector2f(x + w, y + c));
            btn.setPoint(3, sf::Vector2f(x + w, y + h - c));
            btn.setPoint(4, sf::Vector2f(x + w - c, y + h));
            btn.setPoint(5, sf::Vector2f(x + c, y + h));
            btn.setPoint(6, sf::Vector2f(x, y + h - c));
            btn.setPoint(7, sf::Vector2f(x, y + c));

            btn.setOrigin(x + w / 2.0f, y + h / 2.0f);
            btn.setPosition(x + w / 2.0f, y + h / 2.0f);
            btn.setScale(scale, scale);

            sf::Color fillCol;
            sf::Color outCol;

            if (active) {
                fillCol = isRuby ? SunsetCoral : SunsetAmber;
                outCol = SunsetGold;
            }
            else if (hovered) {
                fillCol = isRuby ? SunsetCoralDark : SunsetPlum;
                outCol = SunsetPeach;
            }
            else {
                fillCol = isRuby ? sf::Color(110, 24, 48) : SunsetSkyMid;
                outCol = SunsetCoralDark;
            }

            btn.setFillColor(fillCol);
            btn.setOutlineThickness(1.0f);
            btn.setOutlineColor(outCol);
            window.draw(btn);

            sf::RectangleShape hShelf(sf::Vector2f(w - c * 2.0f, 1.0f));
            hShelf.setOrigin((w - c * 2.0f) / 2.0f, 0.5f);
            hShelf.setPosition(x + w / 2.0f, y + 1.5f);
            hShelf.setScale(scale, scale);
            hShelf.setFillColor(active ? sf::Color(255, 255, 255, 170) : sf::Color(255, 255, 255, 50));
            window.draw(hShelf);

            if (!label.empty()) {
                sf::Color textColor = active ? (isRuby ? sf::Color::White : SunsetDeepDark) : (hovered ? SunsetGold : TextPrimary);
                sf::Color textShadow = (active && !isRuby) ? Transparent : sf::Color(14, 6, 20, 230);
                DrawCrispText(window, font, label, charSize, x + w / 2.0f, y + h / 2.0f, textColor, textShadow, true, true);
            }
        }

        static void DrawThemedButton(sf::RenderWindow& window, sf::FloatRect bounds, const std::string& label, const sf::Font& font, unsigned int charSize, bool active, bool hovered, bool isRuby = false, float scale = 1.0f) {
            DrawSunsetButton(window, bounds, label, font, charSize, active, hovered, isRuby, scale);
        }

        static void DrawLoFiButton(sf::RenderWindow& window, sf::FloatRect bounds, const std::string& label, const sf::Font& font, unsigned int charSize, bool active, bool hovered, bool isRuby = false, float scale = 1.0f) {
            DrawSunsetButton(window, bounds, label, font, charSize, active, hovered, isRuby, scale);
        }

        static void DrawLoFiPanel(sf::RenderWindow& window, sf::FloatRect bounds, float alphaMult = 1.0f) {
            DrawSunsetPanel(window, bounds, alphaMult);
        }

        static void DrawParchmentPanel(sf::RenderWindow& window, sf::FloatRect bounds, float alphaMult = 1.0f) {
            DrawSunsetPanel(window, bounds, alphaMult);
        }

        static void DrawFiligreePanel(sf::RenderWindow& window, sf::FloatRect bounds, float alphaMult = 1.0f) {
            DrawSunsetPanel(window, bounds, alphaMult);
        }

        static void DrawCarvedWoodPlank(sf::RenderWindow& window, sf::FloatRect bounds, bool vertical = false, float alphaMult = 1.0f) {
            DrawSunsetPanel(window, bounds, alphaMult);
        }

        static void DrawWoodPanel(sf::RenderWindow& window, sf::FloatRect bounds, float alphaMult = 1.0f) {
            DrawSunsetPanel(window, bounds, alphaMult);
        }

        static void DrawCornerBrackets(sf::RenderWindow& window, sf::FloatRect bounds, float alphaMult = 1.0f) {
            float x = std::floor(bounds.left);
            float y = std::floor(bounds.top);
            float w = std::floor(bounds.width);
            float h = std::floor(bounds.height);
            float c = 6.0f;

            auto drawDiamond = [&](float px, float py) {
                sf::ConvexShape d(4);
                d.setPoint(0, sf::Vector2f(0.0f, -2.5f));
                d.setPoint(1, sf::Vector2f(2.5f, 0.0f));
                d.setPoint(2, sf::Vector2f(0.0f, 2.5f));
                d.setPoint(3, sf::Vector2f(-2.5f, 0.0f));
                d.setPosition(std::floor(px), std::floor(py));
                sf::Color dc = SunsetAmber;
                dc.a = static_cast<sf::Uint8>(240 * alphaMult);
                d.setFillColor(dc);
                window.draw(d);
                };

            drawDiamond(x + c, y + c);
            drawDiamond(x + w - c, y + c);
            drawDiamond(x + c, y + h - c);
            drawDiamond(x + w - c, y + h - c);
        }
    };

}