#include "ToolDock.h"
#include "../UITheme.h"
#include "../UIIcons.h"
#include <cmath>
#include <algorithm>

namespace WisdomUI {

    ToolDock::ToolDock() = default;

    void ToolDock::Initialize(const sf::Font& font) {
        m_font = font;
    }

    void ToolDock::SetBounds(const sf::FloatRect& bounds) {
        m_bounds = bounds;

        float availableHeight = bounds.height - 20.0f;
        float btnSize = 40.0f;
        float gap = 6.0f;

        if (!m_tools.empty()) {
            float totalNeeded = static_cast<float>(m_tools.size()) * btnSize + static_cast<float>(m_tools.size() - 1) * gap;
            if (totalNeeded > availableHeight && availableHeight > 100.0f) {
                btnSize = std::floor((availableHeight - static_cast<float>(m_tools.size() - 1) * 3.0f) / static_cast<float>(m_tools.size()));
                btnSize = std::clamp(btnSize, 26.0f, 40.0f);
                gap = 3.0f;
            }
        }

        float startY = bounds.top + 10.0f;
        float startX = bounds.left + (bounds.width - btnSize) / 2.0f;

        for (auto& tool : m_tools) {
            tool.bounds = sf::FloatRect(std::floor(startX), std::floor(startY), btnSize, btnSize);
            if (tool.id == m_activeToolId) {
                m_selectionSliderY = startY;
                m_targetSelectionSliderY = startY;
            }
            startY += btnSize + gap;
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
                m_targetSelectionSliderY = tool.bounds.top;
                break;
            }
        }
    }

    void ToolDock::Update(float deltaTime, const sf::Vector2f& mousePos) {
        m_globalTime += deltaTime;
        m_selectionSliderY += (m_targetSelectionSliderY - m_selectionSliderY) * 18.0f * deltaTime;

        bool hasHover = false;
        for (auto& tool : m_tools) {
            bool isHov = tool.bounds.contains(mousePos);
            tool.hoverAlpha += ((isHov ? 1.0f : 0.0f) - tool.hoverAlpha) * 14.0f * deltaTime;
            tool.scale += ((isHov ? 1.08f : 1.0f) - tool.scale) * 16.0f * deltaTime;

            if (isHov) {
                hasHover = true;
                m_hoveredTooltip = tool.tooltip;

                sf::Text dummy(tool.tooltip, m_font, 14);
                float tw = dummy.getLocalBounds().width + 28.0f;
                float th = 34.0f;
                float tipX = std::floor(m_bounds.left + m_bounds.width + 12.0f);
                float tipY = std::floor(tool.bounds.top + (tool.bounds.height - th) / 2.0f);
                tipY = std::clamp(tipY, 10.0f, 1080.0f - th - 10.0f);

                m_tooltipPos = sf::Vector2f(tipX, tipY);
            }
        }

        m_tooltipAlpha += ((hasHover ? 1.0f : 0.0f) - m_tooltipAlpha) * 16.0f * deltaTime;
    }

    bool ToolDock::HandleEvent(const sf::Event& event, const sf::RenderWindow& window) {
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            sf::Vector2f mousePos = window.mapPixelToCoords({ event.mouseButton.x, event.mouseButton.y });
            for (auto& tool : m_tools) {
                if (tool.bounds.contains(mousePos)) {
                    if (m_activeToolId == tool.id) {
                        m_activeToolId = "";
                        if (m_onDeselect) m_onDeselect();
                        return true;
                    }
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
        Theme::DrawSunsetPanel(window, m_bounds, 1.0f);

        if (!m_tools.empty() && !m_activeToolId.empty()) {
            float btnSize = m_tools.front().bounds.width;
            float startX = m_bounds.left + (m_bounds.width - btnSize) / 2.0f;
            sf::FloatRect activeIndicatorBounds(std::floor(startX), std::floor(m_selectionSliderY), btnSize, btnSize);
            Theme::DrawSunsetButton(window, activeIndicatorBounds, "", m_font, 11, true, false, true, 1.0f);

            float pulseGlow = std::sin(m_globalTime * 3.5f) * 0.25f + 0.65f;
            sf::RectangleShape glowRibbon(sf::Vector2f(3.0f, btnSize - 6.0f));
            glowRibbon.setPosition(std::floor(m_bounds.left + 3.0f), std::floor(m_selectionSliderY + 3.0f));
            sf::Color glowCol = Theme::SunsetAmber;
            glowCol.a = static_cast<sf::Uint8>(255 * pulseGlow);
            glowRibbon.setFillColor(glowCol);
            window.draw(glowRibbon);
        }

        for (const auto& tool : m_tools) {
            bool isActive = (m_activeToolId == tool.id);

            if (!isActive) {
                Theme::DrawSunsetButton(window, tool.bounds, "", m_font, 11, false, tool.hoverAlpha > 0.5f, false, tool.scale);
            }

            float iconSize = std::clamp(tool.bounds.width - 16.0f, 16.0f, 24.0f);
            float iconX = std::floor(tool.bounds.left + (tool.bounds.width - iconSize) / 2.0f);
            float iconY = std::floor(tool.bounds.top + (tool.bounds.height - iconSize) / 2.0f);

            sf::Color iconColor = isActive ? sf::Color::White : (tool.hoverAlpha > 0.5f ? Theme::SunsetAmber : Theme::TextSecondary);
            Icons::Draw(window, tool.id, sf::Vector2f(iconX, iconY), iconSize, iconColor);
        }
    }

    void ToolDock::RenderTooltip(sf::RenderWindow& window) {
        if (m_tooltipAlpha > 0.02f) {
            sf::Text dummy(m_hoveredTooltip, m_font, 14);
            float tw = dummy.getLocalBounds().width + 28.0f;
            float th = 34.0f;

            sf::FloatRect tipBounds(m_tooltipPos.x, m_tooltipPos.y, tw, th);
            Theme::DrawSunsetPanel(window, tipBounds, m_tooltipAlpha);

            sf::Color tipTextCol = Theme::SunsetGold;
            tipTextCol.a = static_cast<sf::Uint8>(255 * m_tooltipAlpha);
            sf::Color shadowCol = sf::Color(14, 6, 20, static_cast<sf::Uint8>(230 * m_tooltipAlpha));

            Theme::DrawCrispText(window, m_font, m_hoveredTooltip, 14, tipBounds.left + tw / 2.0f, tipBounds.top + th / 2.0f, tipTextCol, shadowCol, true, true);
        }
    }

}