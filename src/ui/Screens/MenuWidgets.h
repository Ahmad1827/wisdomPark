#pragma once
#include <SFML/Graphics.hpp>
#include "../UITheme.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numeric>
#include <string>

// Small drawing helpers shared by the start-menu screens.
namespace MenuWidgets {

    inline const sf::Color kTextShadow(10, 4, 18);

    inline sf::Color faded(sf::Color color, float alpha) {
        color.a = static_cast<sf::Uint8>(static_cast<float>(color.a) * std::clamp(alpha, 0.0f, 1.0f));
        return color;
    }

    inline sf::FloatRect shifted(sf::FloatRect rect, float dx, float dy) {
        rect.left += dx;
        rect.top += dy;
        return rect;
    }

    // Translucent card that brightens and gains a halo as hoverT goes from 0 to 1.
    inline void drawCard(sf::RenderWindow& window, const sf::FloatRect& b, float hoverT, float alpha) {
        using WisdomUI::Theme;

        if (hoverT > 0.01f) {
            sf::ConvexShape glow = Theme::ChamferedRect(b.left - 4.0f, b.top - 4.0f, b.width + 8.0f, b.height + 8.0f, 8.0f);
            glow.setFillColor(Theme::WithAlpha(Theme::SunsetPeach, 55.0f * hoverT * alpha));
            window.draw(glow);
        }

        sf::ConvexShape dropShadow = Theme::ChamferedRect(b.left, b.top + 5.0f + 3.0f * hoverT, b.width, b.height, 6.0f);
        dropShadow.setFillColor(faded(sf::Color(6, 3, 12, 150), alpha));
        window.draw(dropShadow);

        sf::ConvexShape card = Theme::ChamferedRect(b.left, b.top, b.width, b.height, 6.0f);
        card.setFillColor(faded(Theme::Mix(sf::Color(22, 14, 36, 236), sf::Color(44, 28, 66, 246), hoverT), alpha));
        card.setOutlineThickness(1.0f);
        card.setOutlineColor(faded(Theme::Mix(Theme::Border, Theme::SunsetPeach, hoverT), alpha));
        window.draw(card);

        sf::RectangleShape shine(sf::Vector2f(b.width - 12.0f, 1.0f));
        shine.setPosition(b.left + 6.0f, b.top + 1.0f);
        shine.setFillColor(faded(sf::Color(255, 255, 255, 34), alpha));
        window.draw(shine);
    }

    inline void drawDiamond(sf::RenderWindow& window, float cx, float cy, float radius, sf::Color color) {
        sf::ConvexShape d(4);
        d.setPoint(0, sf::Vector2f(0.0f, -radius));
        d.setPoint(1, sf::Vector2f(radius, 0.0f));
        d.setPoint(2, sf::Vector2f(0.0f, radius));
        d.setPoint(3, sf::Vector2f(-radius, 0.0f));
        d.setPosition(std::floor(cx), std::floor(cy));
        d.setFillColor(color);
        window.draw(d);
    }

    // Amber caption in the top-left of a card, with a thin rule running to its right edge.
    inline void drawCardTitle(sf::RenderWindow& window, const sf::Font& font, const std::string& title, const sf::FloatRect& card, float alpha) {
        using WisdomUI::Theme;

        const float y = card.top + 30.0f;
        Theme::DrawCrispText(window, font, title, 16, card.left + 22.0f, y, faded(Theme::SunsetAmber, alpha), faded(kTextShadow, alpha), false, true);

        float titleW = Theme::MeasureText(font, title, 16);
        sf::RectangleShape rule(sf::Vector2f(std::max(0.0f, card.width - titleW - 58.0f), 1.0f));
        rule.setPosition(card.left + 22.0f + titleW + 14.0f, y);
        rule.setFillColor(Theme::WithAlpha(Theme::SunsetPlum, 255.0f * alpha));
        window.draw(rule);
    }

    // Light checkerboard behind artwork thumbnails so transparent pixels stay readable.
    inline void drawCheckerboard(sf::RenderWindow& window, const sf::FloatRect& area, float cell, float alpha) {
        sf::RectangleShape base(sf::Vector2f(area.width, area.height));
        base.setPosition(area.left, area.top);
        base.setFillColor(faded(sf::Color(214, 208, 220), alpha));
        window.draw(base);

        sf::RectangleShape chk;
        chk.setFillColor(faded(sf::Color(184, 176, 194), alpha));
        int row = 0;
        for (float y = area.top; y < area.top + area.height - 0.5f; y += cell, ++row) {
            int col = 0;
            for (float x = area.left; x < area.left + area.width - 0.5f; x += cell, ++col) {
                if ((row + col) % 2 == 0) continue;
                chk.setSize(sf::Vector2f(std::min(cell, area.left + area.width - x), std::min(cell, area.top + area.height - y)));
                chk.setPosition(x, y);
                window.draw(chk);
            }
        }
    }

    // A view that only lets drawing through inside `area`, in the same coordinates as the current view.
    inline sf::View clipView(const sf::RenderWindow& window, const sf::FloatRect& area) {
        const sf::View& current = window.getView();
        sf::FloatRect vp = current.getViewport();
        sf::Vector2f size = current.getSize();
        sf::Vector2f origin = current.getCenter() - size * 0.5f;

        sf::View clip(area);
        clip.setViewport(sf::FloatRect(
            vp.left + ((area.left - origin.x) / size.x) * vp.width,
            vp.top + ((area.top - origin.y) / size.y) * vp.height,
            (area.width / size.x) * vp.width,
            (area.height / size.y) * vp.height));
        return clip;
    }

    inline void drawScrollbar(sf::RenderWindow& window, const sf::FloatRect& track, float scroll, float maxScroll, float visibleRatio) {
        using WisdomUI::Theme;
        if (maxScroll <= 0.0f) return;

        sf::RectangleShape rail(sf::Vector2f(track.width, track.height));
        rail.setPosition(track.left, track.top);
        rail.setFillColor(sf::Color(10, 6, 16, 180));
        window.draw(rail);

        float thumbH = std::max(36.0f, track.height * std::clamp(visibleRatio, 0.0f, 1.0f));
        sf::RectangleShape thumb(sf::Vector2f(track.width, thumbH));
        thumb.setPosition(track.left, track.top + (scroll / maxScroll) * (track.height - thumbH));
        thumb.setFillColor(Theme::SunsetAmber);
        window.draw(thumb);
    }

    // ---- Modals -------------------------------------------------------------------------

    // Modals drawn straight from a flag have no open() call, so their entrance is timed from
    // the first frame they are drawn again. Returns the seconds since the modal appeared.
    struct ModalClock {
        unsigned int lastDrawnFrame = 0;
        float openTime = 0.0f;

        float tick() {
            using WisdomUI::Theme;
            if (Theme::s_frame - lastDrawnFrame > 1) openTime = Theme::s_time;
            lastDrawnFrame = Theme::s_frame;
            return Theme::s_time - openTime;
        }
    };

    inline float modalAppear(float elapsed) {
        return WisdomUI::Animation::EaseOutCubic(elapsed / 0.22f);
    }

    // Dims the screen and starts the rise-into-place entrance by shifting the view, which
    // keeps every bound unchanged. Returns the view to restore once the modal is drawn.
    inline sf::View beginModal(sf::RenderWindow& window, float appear) {
        sf::VertexArray scrim(sf::Quads, 4);
        sf::Color scrimTop = faded(sf::Color(12, 6, 22, 236), appear);
        sf::Color scrimBottom = faded(sf::Color(12, 6, 22, 210), appear);
        scrim[0] = sf::Vertex(sf::Vector2f(0.0f, 0.0f), scrimTop);
        scrim[1] = sf::Vertex(sf::Vector2f(1920.0f, 0.0f), scrimTop);
        scrim[2] = sf::Vertex(sf::Vector2f(1920.0f, 1080.0f), scrimBottom);
        scrim[3] = sf::Vertex(sf::Vector2f(0.0f, 1080.0f), scrimBottom);
        window.draw(scrim);

        sf::View savedView = window.getView();
        sf::View risingView = savedView;
        risingView.move(0.0f, -std::floor(22.0f * (1.0f - appear)));
        window.setView(risingView);
        return savedView;
    }

    // Centred title over a rule, for small confirmation dialogs.
    inline void drawDialogHeader(sf::RenderWindow& window, const sf::Font& font, const sf::FloatRect& panel, const std::string& title, sf::Color titleColor, float elapsed) {
        using WisdomUI::Theme;
        using WisdomUI::Animation;

        float cx = panel.left + panel.width * 0.5f;
        Theme::DrawCrispText(window, font, title, 30, cx, panel.top + 46.0f, titleColor, Theme::SunsetCoralDark, true, true);

        sf::RectangleShape rule(sf::Vector2f(panel.width - 72.0f, 1.0f));
        rule.setPosition(panel.left + 36.0f, panel.top + 80.0f);
        rule.setFillColor(Theme::SunsetPlum);
        window.draw(rule);

        float accentW = 180.0f * Animation::EaseOutCubic((elapsed - 0.08f) / 0.4f);
        sf::RectangleShape accent(sf::Vector2f(accentW, 2.0f));
        accent.setPosition(std::floor(cx - accentW * 0.5f), panel.top + 79.0f);
        accent.setFillColor(titleColor);
        window.draw(accent);
    }

    // Filled amber button for the main action of a screen. It dims while disabled.
    inline void drawPrimaryButton(sf::RenderWindow& window, const sf::Font& font, const sf::FloatRect& bounds, const std::string& label, unsigned int charSize, bool hovered, bool enabled = true) {
        using WisdomUI::Theme;

        float t = Theme::AnimateHover(bounds, hovered) * (enabled ? 1.0f : 0.0f);
        bool pressed = enabled && hovered && sf::Mouse::isButtonPressed(sf::Mouse::Left);
        sf::FloatRect b = shifted(bounds, 0.0f, pressed ? 1.0f : 0.0f);

        if (enabled) {
            sf::ConvexShape glow = Theme::ChamferedRect(b.left - 4.0f, b.top - 4.0f, b.width + 8.0f, b.height + 8.0f, 7.0f);
            glow.setFillColor(Theme::WithAlpha(Theme::SunsetGold, 26.0f + 12.0f * std::sin(Theme::s_time * 3.0f) + 46.0f * t));
            window.draw(glow);
        }

        sf::Color face = Theme::Mix(Theme::SunsetAmber, Theme::SunsetGold, t);
        sf::ConvexShape body = Theme::ChamferedRect(b.left, b.top, b.width, b.height, 4.0f);
        body.setFillColor(enabled ? face : Theme::Mix(face, Theme::SunsetDeepDark, 0.62f));
        body.setOutlineThickness(1.0f);
        body.setOutlineColor(enabled ? Theme::SunsetGlow : Theme::SunsetPlum);
        window.draw(body);

        if (enabled) {
            sf::RectangleShape lower(sf::Vector2f(b.width, b.height * 0.5f - 4.0f));
            lower.setPosition(b.left, b.top + b.height * 0.5f);
            lower.setFillColor(sf::Color(120, 50, 10, 34));
            window.draw(lower);
        }

        Theme::DrawCrispText(window, font, label, charSize, b.left + b.width * 0.5f, b.top + b.height * 0.5f, enabled ? Theme::SunsetDeepDark : Theme::TextMuted, sf::Color::Transparent, true, true);
    }

    // "16:9" for sizes that reduce to small whole numbers, otherwise "1.78:1".
    inline std::string aspectLabel(int width, int height) {
        int divisor = std::max(1, std::gcd(width, height));
        int rw = width / divisor;
        int rh = height / divisor;
        if (rw <= 64 && rh <= 64) return std::to_string(rw) + ":" + std::to_string(rh);

        char buf[24];
        std::snprintf(buf, sizeof(buf), "%.2f:1", static_cast<float>(width) / static_cast<float>(std::max(1, height)));
        return buf;
    }

    inline std::string twoDigits(int n) {
        return (n < 10 ? "0" : "") + std::to_string(n);
    }

}
