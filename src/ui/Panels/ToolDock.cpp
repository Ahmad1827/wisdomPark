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
        float btnSize = 34.0f;
        float startX = bounds.left + (bounds.width - btnSize) / 2.0f;

        for (auto& tool : m_tools) {
            tool.bounds = sf::FloatRect(startX, startY, btnSize, btnSize);
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
    }

    void ToolDock::Update(float deltaTime, const sf::Vector2f& mousePos) {
        m_hoveredTooltip = "";
        for (auto& tool : m_tools) {
            tool.isHovered = tool.bounds.contains(mousePos);
            if (tool.isHovered) {
                m_hoveredTooltip = tool.tooltip;
                m_tooltipPos = sf::Vector2f(tool.bounds.left + tool.bounds.width + 10.0f, tool.bounds.top + 6.0f);
            }
        }
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

        sf::RectangleShape border(sf::Vector2f(Theme::BorderThickness, m_bounds.height));
        border.setPosition(m_bounds.left + m_bounds.width - Theme::BorderThickness, m_bounds.top);
        border.setFillColor(Theme::Border);
        window.draw(border);

        for (const auto& tool : m_tools) {
            bool isActive = (m_activeToolId == tool.id);

            sf::RectangleShape btn(sf::Vector2f(tool.bounds.width, tool.bounds.height));
            btn.setPosition(tool.bounds.left, tool.bounds.top);
            btn.setFillColor(isActive ? Theme::Accent : (tool.isHovered ? Theme::PanelHover : Theme::Panel));
            btn.setOutlineThickness(1.0f);
            btn.setOutlineColor(isActive ? Theme::BorderHighlight : (tool.isHovered ? Theme::Border : sf::Color::Transparent));
            window.draw(btn);

            sf::Color iconColor = isActive ? sf::Color::White : (tool.isHovered ? Theme::Gold : Theme::TextPrimary);
            Icons::Draw(window, tool.id, sf::Vector2f(tool.bounds.left + 7.0f, tool.bounds.top + 7.0f), 20.0f, iconColor);
        }

        if (!m_hoveredTooltip.empty()) {
            sf::Text tip(m_hoveredTooltip, m_font, 11);
            sf::FloatRect tb = tip.getLocalBounds();

            sf::RectangleShape tipBg(sf::Vector2f(tb.width + 12.0f, tb.height + 10.0f));
            tipBg.setPosition(m_tooltipPos.x, m_tooltipPos.y);
            tipBg.setFillColor(sf::Color(25, 20, 15, 240));
            tipBg.setOutlineThickness(1.0f);
            tipBg.setOutlineColor(Theme::BorderHighlight);
            window.draw(tipBg);

            tip.setFillColor(Theme::Gold);
            tip.setPosition(m_tooltipPos.x + 6.0f, m_tooltipPos.y + 3.0f);
            window.draw(tip);
        }
    }

}