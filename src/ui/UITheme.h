#pragma once
#include <SFML/Graphics.hpp>
#include "UIAnimation.h"
#include <algorithm>
#include <cmath>

namespace WisdomUI {

    struct Theme {
        static inline const sf::Color WoodDeepShadow = sf::Color(22, 12, 6);
        static inline const sf::Color WoodDark = sf::Color(46, 26, 14);
        static inline const sf::Color WoodMedium = sf::Color(78, 46, 26);
        static inline const sf::Color WoodLight = sf::Color(118, 74, 42);
        static inline const sf::Color WoodHighlight = sf::Color(156, 104, 62);

        static inline const sf::Color Parchment = sf::Color(236, 220, 188);
        static inline const sf::Color ParchmentDark = sf::Color(204, 180, 140);
        static inline const sf::Color ParchmentShadow = sf::Color(168, 142, 104);
        static inline const sf::Color ParchmentInset = sf::Color(190, 166, 128);

        static inline const sf::Color BrassDark = sf::Color(110, 76, 22);
        static inline const sf::Color Brass = sf::Color(180, 130, 44);
        static inline const sf::Color Gold = sf::Color(246, 202, 68);
        static inline const sf::Color GoldHighlight = sf::Color(255, 240, 150);

        static inline const sf::Color RubyDark = sf::Color(120, 24, 18);
        static inline const sf::Color RubyAccent = sf::Color(182, 42, 32);
        static inline const sf::Color RubyHighlight = sf::Color(226, 72, 58);

        static inline const sf::Color TextParchment = sf::Color(44, 26, 12);
        static inline const sf::Color TextParchmentMuted = sf::Color(108, 80, 52);
        static inline const sf::Color TextGold = sf::Color(255, 224, 110);
        static inline const sf::Color TextPrimary = sf::Color(252, 242, 226);
        static inline const sf::Color TextSecondary = sf::Color(218, 190, 150);
        static inline const sf::Color TextMuted = sf::Color(144, 118, 88);
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

        static void DrawRivet(sf::RenderWindow& window, float x, float y, float alphaMult = 1.0f) {
            sf::CircleShape base(2.5f);
            base.setOrigin(2.5f, 2.5f);
            base.setPosition(x, y);
            sf::Color cBase = BrassDark;
            cBase.a = static_cast<sf::Uint8>(cBase.a * alphaMult);
            base.setFillColor(cBase);
            window.draw(base);

            sf::CircleShape head(1.5f);
            head.setOrigin(1.5f, 1.5f);
            head.setPosition(x - 0.5f, y - 0.5f);
            sf::Color cHead = Gold;
            cHead.a = static_cast<sf::Uint8>(cHead.a * alphaMult);
            head.setFillColor(cHead);
            window.draw(head);
        }

        static void DrawCornerOrnament(sf::RenderWindow& window, float x, float y, float sx, float sy, float alphaMult = 1.0f) {
            sf::ConvexShape corner;
            corner.setPointCount(6);
            corner.setPoint(0, sf::Vector2f(0.0f, 0.0f));
            corner.setPoint(1, sf::Vector2f(14.0f * sx, 0.0f));
            corner.setPoint(2, sf::Vector2f(14.0f * sx, 4.0f * sy));
            corner.setPoint(3, sf::Vector2f(4.0f * sx, 4.0f * sy));
            corner.setPoint(4, sf::Vector2f(4.0f * sx, 14.0f * sy));
            corner.setPoint(5, sf::Vector2f(0.0f, 14.0f * sy));
            corner.setPosition(x, y);

            sf::Color c = Gold;
            c.a = static_cast<sf::Uint8>(c.a * alphaMult);
            corner.setFillColor(c);
            corner.setOutlineThickness(1.0f);
            sf::Color oc = BrassDark;
            oc.a = static_cast<sf::Uint8>(oc.a * alphaMult);
            corner.setOutlineColor(oc);
            window.draw(corner);

            DrawRivet(window, x + 7.0f * sx, y + 7.0f * sy, alphaMult);
        }

        static void DrawCarvedWoodPlank(sf::RenderWindow& window, sf::FloatRect bounds, bool vertical = false, float alphaMult = 1.0f) {
            sf::RectangleShape dropShadow(sf::Vector2f(bounds.width + 4.0f, bounds.height + 4.0f));
            dropShadow.setPosition(bounds.left - 2.0f, bounds.top - 2.0f);
            dropShadow.setFillColor(sf::Color(10, 5, 2, static_cast<sf::Uint8>(190 * alphaMult)));
            window.draw(dropShadow);

            sf::RectangleShape base(sf::Vector2f(bounds.width, bounds.height));
            base.setPosition(bounds.left, bounds.top);
            sf::Color wColor = WoodMedium;
            wColor.a = static_cast<sf::Uint8>(wColor.a * alphaMult);
            base.setFillColor(wColor);
            base.setOutlineThickness(2.0f);
            sf::Color bCol = BrassDark;
            bCol.a = static_cast<sf::Uint8>(bCol.a * alphaMult);
            base.setOutlineColor(bCol);
            window.draw(base);

            if (!vertical) {
                float step = 14.0f;
                for (float y = bounds.top + 4.0f; y < bounds.top + bounds.height - 4.0f; y += step) {
                    sf::RectangleShape groove(sf::Vector2f(bounds.width - 8.0f, 1.0f));
                    groove.setPosition(bounds.left + 4.0f, y);
                    sf::Color gCol = WoodDeepShadow;
                    gCol.a = static_cast<sf::Uint8>(90 * alphaMult);
                    groove.setFillColor(gCol);
                    window.draw(groove);

                    sf::RectangleShape highlight(sf::Vector2f(bounds.width - 8.0f, 1.0f));
                    highlight.setPosition(bounds.left + 4.0f, y + 1.0f);
                    sf::Color hCol = WoodHighlight;
                    hCol.a = static_cast<sf::Uint8>(60 * alphaMult);
                    highlight.setFillColor(hCol);
                    window.draw(highlight);
                }
            }
            else {
                float step = 14.0f;
                for (float x = bounds.left + 4.0f; x < bounds.left + bounds.width - 4.0f; x += step) {
                    sf::RectangleShape groove(sf::Vector2f(1.0f, bounds.height - 8.0f));
                    groove.setPosition(x, bounds.top + 4.0f);
                    sf::Color gCol = WoodDeepShadow;
                    gCol.a = static_cast<sf::Uint8>(90 * alphaMult);
                    groove.setFillColor(gCol);
                    window.draw(groove);

                    sf::RectangleShape highlight(sf::Vector2f(1.0f, bounds.height - 8.0f));
                    highlight.setPosition(x + 1.0f, bounds.top + 4.0f);
                    sf::Color hCol = WoodHighlight;
                    hCol.a = static_cast<sf::Uint8>(60 * alphaMult);
                    highlight.setFillColor(hCol);
                    window.draw(highlight);
                }
            }

            sf::RectangleShape hTop(sf::Vector2f(bounds.width - 4.0f, 2.0f));
            hTop.setPosition(bounds.left + 2.0f, bounds.top + 2.0f);
            sf::Color topCol = WoodHighlight;
            topCol.a = static_cast<sf::Uint8>(180 * alphaMult);
            hTop.setFillColor(topCol);
            window.draw(hTop);

            sf::RectangleShape hLeft(sf::Vector2f(2.0f, bounds.height - 4.0f));
            hLeft.setPosition(bounds.left + 2.0f, bounds.top + 2.0f);
            hLeft.setFillColor(topCol);
            window.draw(hLeft);

            sf::RectangleShape inner(sf::Vector2f(bounds.width - 8.0f, bounds.height - 8.0f));
            inner.setPosition(bounds.left + 4.0f, bounds.top + 4.0f);
            inner.setFillColor(Transparent);
            inner.setOutlineThickness(1.0f);
            sf::Color inCol = Brass;
            inCol.a = static_cast<sf::Uint8>(inCol.a * alphaMult);
            inner.setOutlineColor(inCol);
            window.draw(inner);

            DrawCornerOrnament(window, bounds.left + 2.0f, bounds.top + 2.0f, 1.0f, 1.0f, alphaMult);
            DrawCornerOrnament(window, bounds.left + bounds.width - 2.0f, bounds.top + 2.0f, -1.0f, 1.0f, alphaMult);
            DrawCornerOrnament(window, bounds.left + 2.0f, bounds.top + bounds.height - 2.0f, 1.0f, -1.0f, alphaMult);
            DrawCornerOrnament(window, bounds.left + bounds.width - 2.0f, bounds.top + bounds.height - 2.0f, -1.0f, -1.0f, alphaMult);
        }

        static void DrawParchmentPanel(sf::RenderWindow& window, sf::FloatRect bounds, float alphaMult = 1.0f) {
            DrawCarvedWoodPlank(window, bounds, false, alphaMult);

            sf::FloatRect pb(bounds.left + 8.0f, bounds.top + 8.0f, bounds.width - 16.0f, bounds.height - 16.0f);
            sf::RectangleShape pBg(sf::Vector2f(pb.width, pb.height));
            pBg.setPosition(pb.left, pb.top);
            sf::Color pCol = Parchment;
            pCol.a = static_cast<sf::Uint8>(255 * alphaMult);
            pBg.setFillColor(pCol);
            pBg.setOutlineThickness(1.0f);
            sf::Color pbCol = ParchmentShadow;
            pbCol.a = static_cast<sf::Uint8>(255 * alphaMult);
            pBg.setOutlineColor(pbCol);
            window.draw(pBg);

            sf::RectangleShape vTop(sf::Vector2f(pb.width, 4.0f));
            vTop.setPosition(pb.left, pb.top);
            vTop.setFillColor(sf::Color(140, 110, 70, static_cast<sf::Uint8>(90 * alphaMult)));
            window.draw(vTop);

            sf::RectangleShape vBot(sf::Vector2f(pb.width, 4.0f));
            vBot.setPosition(pb.left, pb.top + pb.height - 4.0f);
            vBot.setFillColor(sf::Color(140, 110, 70, static_cast<sf::Uint8>(90 * alphaMult)));
            window.draw(vBot);

            sf::RectangleShape pinLine(sf::Vector2f(pb.width - 8.0f, 1.0f));
            pinLine.setPosition(pb.left + 4.0f, pb.top + 3.0f);
            pinLine.setFillColor(sf::Color(160, 120, 75, static_cast<sf::Uint8>(120 * alphaMult)));
            window.draw(pinLine);
        }

        static void DrawThemedButton(sf::RenderWindow& window, sf::FloatRect bounds, const std::string& label, const sf::Font& font, int charSize, bool active, bool hovered, bool isRuby = false, float scale = 1.0f) {
            sf::RectangleShape btn(sf::Vector2f(bounds.width, bounds.height));
            btn.setOrigin(bounds.width / 2.0f, bounds.height / 2.0f);
            btn.setPosition(bounds.left + bounds.width / 2.0f, bounds.top + bounds.height / 2.0f);
            btn.setScale(scale, scale);

            sf::Color baseCol;
            if (active) {
                baseCol = isRuby ? RubyHighlight : Gold;
            }
            else if (hovered) {
                baseCol = isRuby ? RubyAccent : WoodHighlight;
            }
            else {
                baseCol = isRuby ? RubyDark : WoodMedium;
            }

            btn.setFillColor(baseCol);
            btn.setOutlineThickness(1.5f);
            btn.setOutlineColor(active ? GoldHighlight : (hovered ? Gold : Brass));
            window.draw(btn);

            sf::RectangleShape h(sf::Vector2f(bounds.width - 4.0f, 2.0f));
            h.setOrigin((bounds.width - 4.0f) / 2.0f, 1.0f);
            h.setPosition(bounds.left + bounds.width / 2.0f, bounds.top + 3.0f);
            h.setScale(scale, scale);
            h.setFillColor(active ? sf::Color(255, 255, 255, 140) : sf::Color(255, 255, 255, 60));
            window.draw(h);

            if (!label.empty()) {
                sf::Text shadow(label, font, charSize);
                sf::FloatRect tb = shadow.getLocalBounds();
                shadow.setOrigin(tb.left + tb.width / 2.0f, tb.top + tb.height / 2.0f);
                shadow.setPosition(bounds.left + bounds.width / 2.0f + 1.0f, bounds.top + bounds.height / 2.0f + 1.0f);
                shadow.setScale(scale, scale);
                shadow.setFillColor(sf::Color(10, 5, 2, 220));
                window.draw(shadow);

                sf::Text txt(label, font, charSize);
                txt.setOrigin(tb.left + tb.width / 2.0f, tb.top + tb.height / 2.0f);
                txt.setPosition(bounds.left + bounds.width / 2.0f, bounds.top + bounds.height / 2.0f);
                txt.setScale(scale, scale);
                txt.setFillColor(active ? sf::Color::White : (hovered ? GoldHighlight : TextPrimary));
                window.draw(txt);
            }
        }

        static void DrawFiligreePanel(sf::RenderWindow& window, sf::FloatRect bounds, float alphaMult = 1.0f) {
            DrawCarvedWoodPlank(window, bounds, false, alphaMult);
        }
    };

}