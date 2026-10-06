#pragma once
#include <SFML/Graphics.hpp>
#include "UIAnimation.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <unordered_map>

namespace WisdomUI {

    struct Theme {
        // Surfaces run from near-black indigo up to violet so panels read clearly
        // against the warm, saturated background art. Warm tones are kept for accents.
        static inline const sf::Color SunsetDeepDark = sf::Color(15, 9, 24);
        static inline const sf::Color SunsetSkyTop = sf::Color(29, 19, 46);
        static inline const sf::Color SunsetSkyMid = sf::Color(58, 36, 84);
        static inline const sf::Color SunsetPlum = sf::Color(96, 60, 124);
        static inline const sf::Color SunsetViolet = sf::Color(150, 96, 180);
        static inline const sf::Color SunsetCoralDark = sf::Color(172, 50, 76);
        static inline const sf::Color SunsetCoral = sf::Color(236, 88, 104);
        static inline const sf::Color SunsetPeach = sf::Color(255, 160, 122);
        static inline const sf::Color SunsetAmber = sf::Color(250, 190, 70);
        static inline const sf::Color SunsetGold = sf::Color(255, 226, 110);
        static inline const sf::Color SunsetGlow = sf::Color(255, 246, 175);

        static inline const sf::Color TextPrimary = sf::Color(255, 247, 238);
        static inline const sf::Color TextSecondary = sf::Color(224, 204, 224);
        static inline const sf::Color TextGold = sf::Color(255, 226, 110);
        static inline const sf::Color TextPeach = sf::Color(255, 174, 140);
        static inline const sf::Color TextMuted = sf::Color(166, 140, 178);
        static inline const sf::Color Transparent = sf::Color(0, 0, 0, 0);

        static inline const sf::Color Background = SunsetDeepDark;
        static inline const sf::Color Panel = SunsetSkyTop;
        static inline const sf::Color PanelHover = SunsetSkyMid;
        static inline const sf::Color PanelInset = SunsetDeepDark;
        static inline const sf::Color Border = sf::Color(118, 72, 138);
        static inline const sf::Color BorderHighlight = SunsetAmber;
        static inline const sf::Color BorderShadow = SunsetDeepDark;
        static inline const sf::Color Accent = SunsetCoral;
        static inline const sf::Color AccentHover = SunsetPeach;
        static inline const sf::Color AccentGlow = sf::Color(255, 160, 122, 90);

        static inline const sf::Color ButtonIdle = SunsetSkyMid;
        static inline const sf::Color ButtonHover = sf::Color(92, 60, 124);
        static inline const sf::Color DangerIdle = sf::Color(104, 26, 52);
        static inline const sf::Color DangerHover = sf::Color(156, 42, 70);

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

        static inline const float TopBarHeight = 50.0f;
        static inline const float OptionsBarHeight = 44.0f;
        static inline const float ToolDockWidth = 58.0f;
        static inline const float RightDockWidth = 300.0f;
        static inline const float TimelineHeight = 210.0f;
        static inline const float TimelineHeaderHeight = 34.0f;
        static inline const float StatusBarHeight = 32.0f;
        static inline const float BorderThickness = 1.0f;

        // Where floating tool panels open: just inside the tool dock and below the bars.
        static inline const float FloatingPanelX = ToolDockWidth + 12.0f;
        static inline const float FloatingPanelY = TopBarHeight + OptionsBarHeight + 10.0f;

        // ---- Per-frame animation state -------------------------------------------------
        // Buttons are drawn in immediate mode, so their hover fade is remembered here,
        // keyed by where the button sits on screen.

        struct HoverState {
            float value = 0.0f;
            unsigned int lastFrame = 0;
        };

        static inline std::unordered_map<std::uint64_t, HoverState> s_hoverStates;
        static inline float s_frameDt = 1.0f / 60.0f;
        static inline float s_time = 0.0f;
        static inline unsigned int s_frame = 0;

        // Call once per frame before any UI is drawn.
        static void BeginFrame(float dt) {
            s_frameDt = std::clamp(dt, 0.0f, 0.1f);
            s_time += s_frameDt;
            ++s_frame;

            if (s_frame % 240 == 0) {
                for (auto it = s_hoverStates.begin(); it != s_hoverStates.end();) {
                    if (s_frame - it->second.lastFrame > 240) it = s_hoverStates.erase(it);
                    else ++it;
                }
            }
        }

        static float AnimateHover(const sf::FloatRect& bounds, bool hovered) {
            if (s_frame == 0) return hovered ? 1.0f : 0.0f;

            auto part = [](float v) { return static_cast<std::uint64_t>(static_cast<std::uint16_t>(static_cast<int>(v))); };
            std::uint64_t key = part(bounds.left) | (part(bounds.top) << 16) | (part(bounds.width) << 32) | (part(bounds.height) << 48);

            HoverState& state = s_hoverStates[key];
            if (state.lastFrame != s_frame) {
                // A button that was not on screen last frame starts from rest.
                if (s_frame - state.lastFrame > 2) state.value = 0.0f;
                float target = hovered ? 1.0f : 0.0f;
                state.value += (target - state.value) * std::min(1.0f, 16.0f * s_frameDt);
                state.lastFrame = s_frame;
            }
            return state.value;
        }

        static sf::Color Mix(const sf::Color& a, const sf::Color& b, float t) {
            return Animation::InterpolateColor(a, b, t);
        }

        static sf::Color WithAlpha(sf::Color color, float alpha) {
            color.a = static_cast<sf::Uint8>(std::clamp(alpha, 0.0f, 255.0f));
            return color;
        }

        static sf::ConvexShape ChamferedRect(float x, float y, float w, float h, float c) {
            sf::ConvexShape shape(8);
            shape.setPoint(0, sf::Vector2f(x + c, y));
            shape.setPoint(1, sf::Vector2f(x + w - c, y));
            shape.setPoint(2, sf::Vector2f(x + w, y + c));
            shape.setPoint(3, sf::Vector2f(x + w, y + h - c));
            shape.setPoint(4, sf::Vector2f(x + w - c, y + h));
            shape.setPoint(5, sf::Vector2f(x + c, y + h));
            shape.setPoint(6, sf::Vector2f(x, y + h - c));
            shape.setPoint(7, sf::Vector2f(x, y + c));
            return shape;
        }

        // ---- Text ----------------------------------------------------------------------

        static float MeasureText(const sf::Font& font, const std::string& str, unsigned int size) {
            sf::Text txt(str, font, size);
            return std::ceil(txt.getLocalBounds().width);
        }

        static void DrawCrispText(sf::RenderWindow& window, const sf::Font& font, const std::string& str, unsigned int size, float x, float y, sf::Color color, sf::Color shadowColor = sf::Color::Transparent, bool centerH = false, bool centerV = false) {
            const_cast<sf::Texture&>(font.getTexture(size)).setSmooth(false);

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

        static void DrawSeparator(sf::RenderWindow& window, float x, float centerY, float height) {
            sf::RectangleShape line(sf::Vector2f(1.0f, height));
            line.setPosition(std::floor(x), std::floor(centerY - height * 0.5f));
            line.setFillColor(SunsetPlum);
            window.draw(line);
        }

        // ---- Panels --------------------------------------------------------------------

        static void DrawPixelDiamond(sf::RenderWindow& window, float px, float py, float alphaMult) {
            sf::ConvexShape d(4);
            d.setPoint(0, sf::Vector2f(0.0f, -2.5f));
            d.setPoint(1, sf::Vector2f(2.5f, 0.0f));
            d.setPoint(2, sf::Vector2f(0.0f, 2.5f));
            d.setPoint(3, sf::Vector2f(-2.5f, 0.0f));
            d.setPosition(std::floor(px), std::floor(py));
            d.setFillColor(WithAlpha(SunsetAmber, 220.0f * alphaMult));
            window.draw(d);
        }

        static void DrawSunsetPanel(sf::RenderWindow& window, sf::FloatRect bounds, float alphaMult = 1.0f) {
            float x = std::floor(bounds.left);
            float y = std::floor(bounds.top);
            float w = std::floor(bounds.width);
            float h = std::floor(bounds.height);
            float c = (std::min(w, h) < 40.0f) ? 4.0f : 6.0f;

            // Soft drop shadow built from three widening layers.
            for (int i = 3; i >= 1; --i) {
                float grow = static_cast<float>(i) * 1.5f;
                sf::ConvexShape shadow = ChamferedRect(x - grow, y - grow + 2.0f, w + grow * 2.0f, h + grow * 2.0f, c + grow);
                shadow.setFillColor(sf::Color(6, 3, 12, static_cast<sf::Uint8>(46.0f * alphaMult)));
                window.draw(shadow);
            }

            sf::ConvexShape base = ChamferedRect(x, y, w, h, c);
            base.setFillColor(WithAlpha(Panel, 246.0f * alphaMult));
            base.setOutlineThickness(1.0f);
            base.setOutlineColor(WithAlpha(Border, 255.0f * alphaMult));
            window.draw(base);

            sf::RectangleShape topEdge(sf::Vector2f(w - c * 2.0f, 1.0f));
            topEdge.setPosition(x + c, y + 1.0f);
            topEdge.setFillColor(WithAlpha(SunsetPeach, 80.0f * alphaMult));
            window.draw(topEdge);

            // Corner studs only on full-size panels; on bars and tooltips they are noise.
            if (w >= 160.0f && h >= 160.0f) {
                DrawPixelDiamond(window, x + c + 2.0f, y + c + 2.0f, alphaMult);
                DrawPixelDiamond(window, x + w - c - 2.0f, y + c + 2.0f, alphaMult);
                DrawPixelDiamond(window, x + c + 2.0f, y + h - c - 2.0f, alphaMult);
                DrawPixelDiamond(window, x + w - c - 2.0f, y + h - c - 2.0f, alphaMult);
            }
        }

        // ---- Buttons -------------------------------------------------------------------

        static void DrawSunsetButton(sf::RenderWindow& window, sf::FloatRect bounds, const std::string& label, const sf::Font& font, unsigned int charSize, bool active, bool hovered, bool isRuby = false, float scale = 1.0f) {
            float x = std::floor(bounds.left);
            float y = std::floor(bounds.top);
            float w = std::floor(bounds.width);
            float h = std::floor(bounds.height);
            float c = (std::min(w, h) < 24.0f) ? 3.0f : 4.0f;

            float hoverT = AnimateHover(bounds, hovered);
            bool pressed = hovered && sf::Mouse::isButtonPressed(sf::Mouse::Left);
            if (pressed) y += 1.0f;

            sf::RenderStates states;
            states.transform.scale(scale, scale, x + w / 2.0f, y + h / 2.0f);

            sf::Color fillCol;
            sf::Color outCol;
            sf::Color glowCol = isRuby ? SunsetCoral : SunsetAmber;

            if (active) {
                fillCol = isRuby ? SunsetCoral : SunsetAmber;
                outCol = SunsetGold;
            }
            else {
                fillCol = Mix(isRuby ? DangerIdle : ButtonIdle, isRuby ? DangerHover : ButtonHover, hoverT);
                outCol = Mix(isRuby ? SunsetCoralDark : Border, SunsetPeach, hoverT);
            }
            if (pressed) fillCol = Mix(fillCol, SunsetDeepDark, 0.3f);

            if (!pressed) {
                sf::ConvexShape shadow = ChamferedRect(x, y + 2.0f, w, h, c);
                shadow.setFillColor(sf::Color(6, 3, 12, 110));
                window.draw(shadow, states);
            }

            // Active buttons breathe; hovered ones fade a halo in.
            float glowAlpha = active ? (46.0f + 26.0f * std::sin(s_time * 3.0f)) : (48.0f * hoverT);
            if (glowAlpha > 1.0f) {
                sf::ConvexShape glow = ChamferedRect(x - 2.0f, y - 2.0f, w + 4.0f, h + 4.0f, c + 1.0f);
                glow.setFillColor(WithAlpha(active ? glowCol : SunsetPeach, glowAlpha));
                window.draw(glow, states);
            }

            sf::ConvexShape btn = ChamferedRect(x, y, w, h, c);
            btn.setFillColor(fillCol);
            btn.setOutlineThickness(1.0f);
            btn.setOutlineColor(outCol);
            window.draw(btn, states);

            // Darker lower half gives the face a subtle bevel.
            sf::ConvexShape lower(6);
            lower.setPoint(0, sf::Vector2f(x, y + h * 0.5f));
            lower.setPoint(1, sf::Vector2f(x + w, y + h * 0.5f));
            lower.setPoint(2, sf::Vector2f(x + w, y + h - c));
            lower.setPoint(3, sf::Vector2f(x + w - c, y + h));
            lower.setPoint(4, sf::Vector2f(x + c, y + h));
            lower.setPoint(5, sf::Vector2f(x, y + h - c));
            lower.setFillColor(sf::Color(0, 0, 0, active ? 26 : 38));
            window.draw(lower, states);

            sf::RectangleShape topShine(sf::Vector2f(w - c * 2.0f, 1.0f));
            topShine.setPosition(x + c, y + 1.0f);
            topShine.setFillColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(active ? 150.0f : 36.0f + 54.0f * hoverT)));
            window.draw(topShine, states);

            if (!label.empty()) {
                sf::Color textColor = active ? (isRuby ? sf::Color::White : SunsetDeepDark) : Mix(TextPrimary, SunsetGold, hoverT);
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

            DrawPixelDiamond(window, x + c, y + c, alphaMult);
            DrawPixelDiamond(window, x + w - c, y + c, alphaMult);
            DrawPixelDiamond(window, x + c, y + h - c, alphaMult);
            DrawPixelDiamond(window, x + w - c, y + h - c, alphaMult);
        }
    };

}
