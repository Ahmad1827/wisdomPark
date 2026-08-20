#include "RightDockTabs.h"
#include "../UITheme.h"
#include "../UIIcons.h"
#include <cmath>

namespace WisdomUI {

    RightDockTabs::RightDockTabs() = default;

    void RightDockTabs::Initialize(const sf::Font& font,
        std::function<void()> onToggleLayers,
        std::function<void()> onTogglePalette,
        std::function<void()> onToggleProperties,
        std::function<void()> onToggleAssets,
        std::function<void()> onToggleAudio) {
        m_font = font;
        m_tabs.clear();

        m_tabs.push_back({ "layers", "Layers Panel", {}, onToggleLayers, false, 0.0f, 1.0f });
        m_tabs.push_back({ "palette", "Color Palette", {}, onTogglePalette, false, 0.0f, 1.0f });
        m_tabs.push_back({ "properties", "Settings & Properties", {}, onToggleProperties, false, 0.0f, 1.0f });
        m_tabs.push_back({ "assets", "Asset Browser", {}, onToggleAssets, false, 0.0f, 1.0f });
        m_tabs.push_back({ "audio", "Audio Timeline Track", {}, onToggleAudio, false, 0.0f, 1.0f });
    }

    void RightDockTabs::SetBounds(const sf::FloatRect& bounds) {
        m_bounds = bounds;
        float startY = bounds.top + 8.0f;
        float btnSize = 36.0f;
        float startX = bounds.left + (bounds.width - btnSize) / 2.0f;

        for (auto& tab : m_tabs) {
            tab.bounds = sf::FloatRect(std::floor(startX), std::floor(startY), btnSize, btnSize);
            startY += btnSize + 8.0f;
        }
    }

    void RightDockTabs::SetTabState(const std::string& id, bool isToggled) {
        for (auto& tab : m_tabs) {
            if (tab.id == id) {
                tab.isToggled = isToggled;
            }
        }
    }

    void RightDockTabs::Update(float deltaTime, const sf::Vector2f& mousePos) {
        bool hasHover = false;
        for (auto& tab : m_tabs) {
            bool isHov = tab.bounds.contains(mousePos);
            tab.hoverAlpha += ((isHov ? 1.0f : 0.0f) - tab.hoverAlpha) * 14.0f * deltaTime;
            tab.scale += ((isHov ? 1.08f : 1.0f) - tab.scale) * 16.0f * deltaTime;

            if (isHov) {
                hasHover = true;
                m_hoveredTooltip = tab.tooltip;
                sf::Text t(tab.tooltip, m_font, 12);
                float w = t.getLocalBounds().width + 24.0f;
                m_tooltipPos = sf::Vector2f(std::floor(tab.bounds.left - w - 10.0f), std::floor(tab.bounds.top + 4.0f));
            }
        }
        m_tooltipAlpha += ((hasHover ? 1.0f : 0.0f) - m_tooltipAlpha) * 16.0f * deltaTime;
    }

    bool RightDockTabs::HandleEvent(const sf::Event& event, const sf::RenderWindow& window) {
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            sf::Vector2f mousePos = window.mapPixelToCoords({ event.mouseButton.x, event.mouseButton.y });
            for (auto& tab : m_tabs) {
                if (tab.bounds.contains(mousePos)) {
                    tab.isToggled = !tab.isToggled;
                    if (tab.onToggle) tab.onToggle();
                    return true;
                }
            }
            if (m_bounds.contains(mousePos)) return true;
        }
        return false;
    }

    void RightDockTabs::Render(sf::RenderWindow& window) {
        Theme::DrawSunsetPanel(window, m_bounds, 1.0f);

        for (const auto& tab : m_tabs) {
            Theme::DrawSunsetButton(window, tab.bounds, "", m_font, 11, tab.isToggled, tab.hoverAlpha > 0.5f, tab.isToggled, tab.scale);

            sf::Color iconCol = tab.isToggled ? sf::Color::White : (tab.hoverAlpha > 0.5f ? Theme::SunsetAmber : Theme::TextSecondary);
            Icons::Draw(window, tab.id, sf::Vector2f(std::floor(tab.bounds.left + 8.0f), std::floor(tab.bounds.top + 8.0f)), 20.0f, iconCol);
        }

        if (m_tooltipAlpha > 0.02f) {
            sf::Text t(m_hoveredTooltip, m_font, 12);
            float tw = t.getLocalBounds().width + 24.0f;
            float th = 28.0f;

            sf::FloatRect tipBounds(m_tooltipPos.x, m_tooltipPos.y, tw, th);
            Theme::DrawSunsetPanel(window, tipBounds, m_tooltipAlpha);

            sf::Color tipTextCol = Theme::SunsetGold;
            tipTextCol.a = static_cast<sf::Uint8>(255 * m_tooltipAlpha);
            sf::Color shadowCol = sf::Color(14, 6, 20, static_cast<sf::Uint8>(230 * m_tooltipAlpha));

            Theme::DrawCrispText(window, m_font, m_hoveredTooltip, 12, tipBounds.left + tw / 2.0f, tipBounds.top + th / 2.0f, tipTextCol, shadowCol, true, true);
        }
    }

}