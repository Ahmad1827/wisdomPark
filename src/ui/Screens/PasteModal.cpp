// Paste sizing choice shown when an image is pasted onto a pixel canvas.
// Drawing and click handling share the layout helpers below.
#include "../UIManager.h"
#include "../UITheme.h"
#include "MenuWidgets.h"
#include <cmath>
#include <string>

using WisdomUI::Theme;
using namespace MenuWidgets;

namespace {

    sf::FloatRect panelBounds() {
        const float w = 580.0f;
        const float h = 372.0f;
        return sf::FloatRect(std::floor((1920.0f - w) * 0.5f), std::floor((1080.0f - h) * 0.5f), w, h);
    }

    sf::FloatRect downscaleButton() { sf::FloatRect p = panelBounds(); return sf::FloatRect(p.left + 36.0f, p.top + 148.0f, p.width - 72.0f, 66.0f); }
    sf::FloatRect originalButton() { sf::FloatRect p = panelBounds(); return sf::FloatRect(p.left + 36.0f, p.top + 226.0f, p.width - 72.0f, 66.0f); }
    sf::FloatRect cancelButton() { sf::FloatRect p = panelBounds(); return sf::FloatRect(p.left + p.width - 36.0f - 130.0f, p.top + 308.0f, 130.0f, 40.0f); }

    ModalClock s_clock;

    // Two-line choice: what it is called, and what it does to the image.
    void drawChoice(sf::RenderWindow& window, const sf::Font& font, const sf::FloatRect& bounds, const std::string& title, const std::string& note, bool hovered) {
        float t = Theme::AnimateHover(bounds, hovered);
        bool pressed = hovered && sf::Mouse::isButtonPressed(sf::Mouse::Left);
        sf::FloatRect b = shifted(bounds, 0.0f, pressed ? 1.0f : 0.0f);

        if (t > 0.01f) {
            sf::ConvexShape glow = Theme::ChamferedRect(b.left - 3.0f, b.top - 3.0f, b.width + 6.0f, b.height + 6.0f, 6.0f);
            glow.setFillColor(Theme::WithAlpha(Theme::SunsetPeach, 48.0f * t));
            window.draw(glow);
        }

        sf::ConvexShape body = Theme::ChamferedRect(b.left, b.top, b.width, b.height, 4.0f);
        body.setFillColor(Theme::Mix(sf::Color(22, 14, 36, 240), sf::Color(52, 32, 76, 248), t));
        body.setOutlineThickness(1.0f);
        body.setOutlineColor(Theme::Mix(Theme::Border, Theme::SunsetPeach, t));
        window.draw(body);

        // A marker grows out of the left edge as the choice is hovered.
        if (t > 0.02f) {
            float barH = (b.height - 16.0f) * t;
            sf::RectangleShape bar(sf::Vector2f(3.0f, barH));
            bar.setPosition(b.left + 1.0f, std::floor(b.top + (b.height - barH) * 0.5f));
            bar.setFillColor(Theme::SunsetAmber);
            window.draw(bar);
        }

        float x = b.left + 22.0f + 6.0f * t;
        Theme::DrawCrispText(window, font, title, 19, x, b.top + 23.0f, Theme::Mix(Theme::TextPrimary, Theme::SunsetGold, t), kTextShadow, false, true);
        Theme::DrawCrispText(window, font, note, 15, x, b.top + 46.0f, Theme::Mix(Theme::TextMuted, Theme::TextSecondary, t), sf::Color::Transparent, false, true);
    }

}

void UIManager::drawPasteResolutionModal(sf::RenderWindow& window, Canvas& canvas) {
    float elapsed = s_clock.tick();
    float appear = modalAppear(elapsed);
    sf::View savedView = beginModal(window, appear);

    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    sf::FloatRect panel = panelBounds();
    float cx = panel.left + panel.width * 0.5f;

    Theme::DrawSunsetPanel(window, panel, appear);
    drawDialogHeader(window, font, panel, "PASTE IMAGE", Theme::SunsetGold, elapsed);

    sf::Vector2u image = m_pendingPasteImage.getSize();
    sf::Vector2u target = canvas.getCanvasSize();
    std::string sizes = "IMAGE  " + std::to_string(image.x) + " x " + std::to_string(image.y) + "   |   CANVAS  " + std::to_string(target.x) + " x " + std::to_string(target.y);
    Theme::DrawCrispText(window, font, "Choose import sizing for this pixel canvas", 16, cx, panel.top + 104.0f, Theme::SunsetPeach, kTextShadow, true, true);
    Theme::DrawCrispText(window, font, sizes, 15, cx, panel.top + 128.0f, Theme::TextMuted, sf::Color::Transparent, true, true);

    sf::FloatRect downscale = downscaleButton();
    sf::FloatRect original = originalButton();
    sf::FloatRect cancel = cancelButton();
    drawChoice(window, font, downscale, "Downscale to Fit Canvas", "Shrinks the image so it fits inside the canvas", downscale.contains(mPos));
    drawChoice(window, font, original, "Keep 1:1 Original Resolution", "Pastes the image at its own pixel size", original.contains(mPos));
    Theme::DrawSunsetButton(window, cancel, "CANCEL", font, 15, false, cancel.contains(mPos), false, 1.0f);

    Theme::DrawCrispText(window, font, "ESC  CANCEL", 15, panel.left + 38.0f, cancel.top + cancel.height * 0.5f, Theme::TextMuted, sf::Color::Transparent, false, true);

    window.setView(savedView);
}

bool UIManager::handlePasteResolutionModalEvent(const sf::Event& event, sf::RenderWindow& window, Canvas& canvas, Timeline& timeline) {
    if (!m_showPasteResolutionModal) return false;

    if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
        m_showPasteResolutionModal = false;
        return true;
    }

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

        if (downscaleButton().contains(mousePos)) {
            canvas.pasteImage(m_pendingPasteImage, timeline.getCurrentFrame(), false);
            m_toolDock.SetActiveTool("select");
            showMessage("Pasted Image (Downscaled)", sf::Color::Green);
            m_showPasteResolutionModal = false;
            return true;
        }

        if (originalButton().contains(mousePos)) {
            canvas.pasteImage(m_pendingPasteImage, timeline.getCurrentFrame(), true);
            m_toolDock.SetActiveTool("select");
            showMessage("Pasted Image (Original 1:1)", sf::Color::Green);
            m_showPasteResolutionModal = false;
            return true;
        }

        if (cancelButton().contains(mousePos)) {
            m_showPasteResolutionModal = false;
            return true;
        }

        return true;
    }

    return true;
}
