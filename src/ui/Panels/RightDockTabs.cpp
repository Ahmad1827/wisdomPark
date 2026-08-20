#include "RightDockTabs.h"
#include "../UITheme.h"
#include "../UIIcons.h"

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
            tab.bounds = sf::FloatRect(startX, startY, btnSize, btnSize);
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
            tab.scale += ((isHov ? 1.10f : 1.0f) - tab.scale) * 16.0f * deltaTime;

            if (isHov) {
                hasHover = true;
                m_hoveredTooltip = tab.tooltip;
                sf::Text t(tab.tooltip, m_font, 11);
                float w = t.getLocalBounds().width + 16.0f;
                m_tooltipPos = sf::Vector2f(tab.bounds.left - w - 6.0f, tab.bounds.top + 6.0f);
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
        sf::RectangleShape bg(sf::Vector2f(m_bounds.width, m_bounds.height));
        bg.setPosition(m_bounds.left, m_bounds.top);
        bg.setFillColor(Theme::WoodMedium);
        window.draw(bg);

        sf::RectangleShape border(sf::Vector2f(2.0f, m_bounds.height));
        border.setPosition(m_bounds.left, m_bounds.top);
        border.setFillColor(Theme::Brass);
        window.draw(border);

        for (const auto& tab : m_tabs) {
            sf::RectangleShape btn(sf::Vector2f(tab.bounds.width, tab.bounds.height));
            btn.setOrigin(tab.bounds.width / 2.0f, tab.bounds.height / 2.0f);
            btn.setPosition(tab.bounds.left + tab.bounds.width / 2.0f, tab.bounds.top + tab.bounds.height / 2.0f);
            btn.setScale(tab.scale, tab.scale);

            sf::Color fillCol = tab.isToggled ? Theme::RubyAccent : Animation::InterpolateColor(Theme::WoodDark, Theme::WoodLight, tab.hoverAlpha);
            btn.setFillColor(fillCol);
            btn.setOutlineThickness(1.0f);
            btn.setOutlineColor(tab.isToggled ? Theme::Gold : Animation::InterpolateColor(Theme::Brass, Theme::Gold, tab.hoverAlpha));
            window.draw(btn);

            sf::Color iconCol = tab.isToggled ? sf::Color::White : (tab.hoverAlpha > 0.5f ? Theme::Gold : Theme::Parchment);
            Icons::Draw(window, tab.id, sf::Vector2f(tab.bounds.left + 8.0f, tab.bounds.top + 8.0f), 20.0f, iconCol);
        }

        if (m_tooltipAlpha > 0.02f) {
            sf::Text tip(m_hoveredTooltip, m_font, 11);
            sf::FloatRect tb = tip.getLocalBounds();

            sf::FloatRect tipBounds(m_tooltipPos.x, m_tooltipPos.y, tb.width + 16.0f, tb.height + 12.0f);
            Theme::DrawParchmentPanel(window, tipBounds, m_tooltipAlpha);

            tip.setFillColor(Theme::TextParchment);
            tip.setPosition(m_tooltipPos.x + 8.0f, m_tooltipPos.y + 4.0f);
            window.draw(tip);
        }
    }

}