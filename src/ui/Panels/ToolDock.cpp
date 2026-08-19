#include "ToolDock.h"
#include "../UITheme.h"

namespace WisdomUI {

    ToolDock::ToolDock() {
        m_background.setFillColor(Theme::Panel);
        m_rightBorder.setFillColor(Theme::Border);
    }

    void ToolDock::Initialize(const sf::Texture& iconTexture, const sf::Font& font) {
        m_iconTexture = &iconTexture;
        m_font = &font;
    }

    void ToolDock::SetBounds(const sf::FloatRect& bounds) {
        m_bounds = bounds;
        m_background.setPosition(bounds.left, bounds.top);
        m_background.setSize(sf::Vector2f(bounds.width, bounds.height));

        m_rightBorder.setPosition(bounds.left + bounds.width - Theme::BorderThickness, bounds.top);
        m_rightBorder.setSize(sf::Vector2f(Theme::BorderThickness, bounds.height));
    }

    void ToolDock::AddTool(const ToolDefinition& tool) {
        m_tools.push_back(tool);
    }

    void ToolDock::SetActiveTool(const std::string& id) {
        m_activeToolId = id;
    }

    void ToolDock::Update(float deltaTime, const sf::Vector2f& mousePos) {
        m_hoveredToolId = "";
        if (!m_bounds.contains(mousePos)) return;

        float currentY = m_bounds.top + m_spacing;
        float startX = m_bounds.left + (m_bounds.width - m_buttonSize) / 2.0f;

        for (const auto& tool : m_tools) {
            sf::FloatRect btnBounds(startX, currentY, m_buttonSize, m_buttonSize);
            if (btnBounds.contains(mousePos)) {
                m_hoveredToolId = tool.id;
                break;
            }
            currentY += m_buttonSize + m_spacing;
        }
    }

    bool ToolDock::HandleEvent(const sf::Event& event, const sf::RenderWindow& window) {
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            sf::Vector2f mousePos = window.mapPixelToCoords({ event.mouseButton.x, event.mouseButton.y });

            if (!m_bounds.contains(mousePos)) return false;

            float currentY = m_bounds.top + m_spacing;
            float startX = m_bounds.left + (m_bounds.width - m_buttonSize) / 2.0f;

            for (const auto& tool : m_tools) {
                sf::FloatRect btnBounds(startX, currentY, m_buttonSize, m_buttonSize);
                if (btnBounds.contains(mousePos)) {
                    SetActiveTool(tool.id);
                    if (tool.onSelect) tool.onSelect();
                    return true;
                }
                currentY += m_buttonSize + m_spacing;
            }
        }
        return false;
    }

    void ToolDock::Render(sf::RenderWindow& window) {
        window.draw(m_background);
        window.draw(m_rightBorder);

        float currentY = m_bounds.top + m_spacing;
        float startX = m_bounds.left + (m_bounds.width - m_buttonSize) / 2.0f;

        for (const auto& tool : m_tools) {
            sf::RectangleShape btnShape(sf::Vector2f(m_buttonSize, m_buttonSize));
            btnShape.setPosition(startX, currentY);

            bool isActive = (m_activeToolId == tool.id);
            bool isHovered = (m_hoveredToolId == tool.id);

            if (isActive) {
                btnShape.setFillColor(Theme::Accent);
                btnShape.setOutlineThickness(1.0f);
                btnShape.setOutlineColor(Theme::TextPrimary);
            }
            else if (isHovered) {
                btnShape.setFillColor(Theme::AccentHover);
                btnShape.setOutlineThickness(0.0f);
            }
            else {
                btnShape.setFillColor(Theme::Transparent);
                btnShape.setOutlineThickness(0.0f);
            }

            window.draw(btnShape);

            if (m_iconTexture) {
                sf::Sprite iconSprite(*m_iconTexture, tool.iconRect);
                // Center the icon inside the button
                iconSprite.setPosition(
                    startX + (m_buttonSize - tool.iconRect.width) / 2.0f,
                    currentY + (m_buttonSize - tool.iconRect.height) / 2.0f
                );
                window.draw(iconSprite);
            }

            currentY += m_buttonSize + m_spacing;
        }
    }

}