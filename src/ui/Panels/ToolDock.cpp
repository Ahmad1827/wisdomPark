#include "ToolDock.h"
#include "../UITheme.h"
#include "../UIIcons.h"

namespace WisdomUI {

    ToolDock::ToolDock() = default;

    void ToolDock::Initialize(const sf::Font& font) {
        m_font = font;
    }

    void ToolDock::SetBounds(const sf::FloatRect& bounds) {
        m_bounds = bounds;
        float startY = bounds.top + 8.0f;
        float btnSize = 36.0f;
        float startX = bounds.left + (bounds.width - btnSize) / 2.0f;

        for (auto& tool : m_tools) {
            tool.bounds = sf::FloatRect(startX, startY, btnSize, btnSize);
            if (tool.id == m_activeToolId) {
                m_selectionSliderY.SetImmediate(startY);
            }
            startY += btnSize + 6.0f;
        }
    }

    void ToolDock::AddTool(const std::string& id, const std::string& tooltip, std::function<void()> onSelect) {
        ToolItem item;
        item.id = id;
        item.tooltip = tooltip;
        item.onSelect = onSelect;
        m_tools.push_back(item);
        SetBounds(m_bounds);
    }

    void ToolDock::SetActiveTool(const std::string& id) {
        m_activeToolId = id;
        for (const auto& tool : m_tools) {
            if (tool.id == id) {
                m_selectionSliderY.target = tool.bounds.top;
                break;
            }
        }
    }

    void ToolDock::Update(float deltaTime, const sf::Vector2f& mousePos) {
        m_globalTime += deltaTime;
        m_selectionSliderY.Update(deltaTime);

        bool hasHover = false;
        for (auto& tool : m_tools) {
            bool isHov = tool.bounds.contains(mousePos);
            tool.hoverAlpha += ((isHov ? 1.0f : 0.0f) - tool.hoverAlpha) * 14.0f * deltaTime;
            tool.scale += ((isHov ? 1.10f : 1.0f) - tool.scale) * 16.0f * deltaTime;

            if (isHov) {
                hasHover = true;
                m_hoveredTooltip = tool.tooltip;
                m_tooltipPos = sf::Vector2f(tool.bounds.left + tool.bounds.width + 12.0f, tool.bounds.top + 6.0f);
            }
        }

        m_tooltipAlpha += ((hasHover ? 1.0f : 0.0f) - m_tooltipAlpha) * 16.0f * deltaTime;
    }

    bool ToolDock::HandleEvent(const sf::Event& event, const sf::RenderWindow& window) {
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            sf::Vector2f mousePos = window.mapPixelToCoords({ event.mouseButton.x, event.mouseButton.y });
            for (auto& tool : m_tools) {
                if (tool.bounds.contains(mousePos)) {
                    SetActiveTool(tool.id);
                    if (tool.onSelect) tool.onSelect();
                    return true;
                }
            }
            if (m_bounds.contains(mousePos)) return true;
        }
        return false;
    }

    void ToolDock::Render(sf::RenderWindow& window) {
        sf::RectangleShape bg(sf::Vector2f(m_bounds.width, m_bounds.height));
        bg.setPosition(m_bounds.left, m_bounds.top);
        bg.setFillColor(Theme::Background);
        window.draw(bg);

        sf::RectangleShape border(sf::Vector2f(Theme::BorderThickness * 2.0f, m_bounds.height));
        border.setPosition(m_bounds.left + m_bounds.width - Theme::BorderThickness * 2.0f, m_bounds.top);
        border.setFillColor(Theme::Border);
        window.draw(border);

        if (!m_tools.empty()) {
            float btnSize = 36.0f;
            float startX = m_bounds.left + (m_bounds.width - btnSize) / 2.0f;
            sf::RectangleShape activeIndicator(sf::Vector2f(btnSize, btnSize));
            activeIndicator.setPosition(startX, m_selectionSliderY.current);
            activeIndicator.setFillColor(Theme::Accent);
            activeIndicator.setOutlineThickness(1.0f);
            activeIndicator.setOutlineColor(Theme::BorderHighlight);
            window.draw(activeIndicator);

            float pulseGlow = Animation::Pulse(m_globalTime, 3.5f, 0.4f, 0.9f);
            sf::RectangleShape glowRibbon(sf::Vector2f(3.0f, btnSize - 6.0f));
            glowRibbon.setPosition(m_bounds.left + 2.0f, m_selectionSliderY.current + 3.0f);
            sf::Color glowCol = Theme::BorderHighlight;
            glowCol.a = static_cast<sf::Uint8>(255 * pulseGlow);
            glowRibbon.setFillColor(glowCol);
            window.draw(glowRibbon);
        }

        for (const auto& tool : m_tools) {
            bool isActive = (m_activeToolId == tool.id);

            if (!isActive && tool.hoverAlpha > 0.01f) {
                sf::RectangleShape hovBtn(sf::Vector2f(tool.bounds.width, tool.bounds.height));
                hovBtn.setOrigin(tool.bounds.width / 2.0f, tool.bounds.height / 2.0f);
                hovBtn.setPosition(tool.bounds.left + tool.bounds.width / 2.0f, tool.bounds.top + tool.bounds.height / 2.0f);
                hovBtn.setScale(tool.scale, tool.scale);

                sf::Color hovCol = Theme::PanelHover;
                hovCol.a = static_cast<sf::Uint8>(220 * tool.hoverAlpha);
                hovBtn.setFillColor(hovCol);
                hovBtn.setOutlineThickness(1.0f);
                sf::Color hovBrd = Theme::Border;
                hovBrd.a = static_cast<sf::Uint8>(255 * tool.hoverAlpha);
                hovBtn.setOutlineColor(hovBrd);
                window.draw(hovBtn);
            }

            sf::Color iconColor = isActive ? sf::Color::White : (tool.hoverAlpha > 0.5f ? Theme::Gold : Theme::TextPrimary);
            Icons::Draw(window, tool.id, sf::Vector2f(tool.bounds.left + 8.0f, tool.bounds.top + 8.0f), 20.0f, iconColor);
        }

        if (m_tooltipAlpha > 0.02f) {
            sf::Text tip(m_hoveredTooltip, m_font, 11);
            sf::FloatRect tb = tip.getLocalBounds();

            sf::FloatRect tipBounds(m_tooltipPos.x, m_tooltipPos.y, tb.width + 16.0f, tb.height + 12.0f);
            Theme::DrawFiligreePanel(window, tipBounds, m_tooltipAlpha);

            tip.setFillColor(Theme::Gold);
            tip.setPosition(m_tooltipPos.x + 8.0f, m_tooltipPos.y + 4.0f);
            window.draw(tip);
        }
    }

}