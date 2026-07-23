#include "Panels/WorkspaceEnvironment.h"
#include "Theme.h"

namespace StudioUI {

void WorkspaceEnvironment::InitializeFont(const std::string& fontPath) {
    m_font.loadFromFile(fontPath);
    
    m_statusBarBg.setFillColor(Theme::PanelBackground);
    m_statusBarTopBorder.setFillColor(Theme::BorderColor);
    m_statusBarTopBorder.setSize({1280.0f, Theme::BorderThickness});

    m_statusText.setFont(m_font);
    m_statusText.setCharacterSize(11);
    m_statusText.setFillColor(Theme::TextSecondary);
}

void WorkspaceEnvironment::UpdateStatusBar(float zoom, sf::Vector2f mousePos, int totalSprites, int selectedCount, const std::string& statusText) {
    // Format: Ready | Zoom 100% | Mouse 235,421 | Sprites 58 | Selected 3 | Autosaved
    std::string statusStr = statusText + 
                            "  |  Zoom " + std::to_string(static_cast<int>(zoom * 100)) + "%" +
                            "  |  Mouse " + std::to_string(static_cast<int>(mousePos.x)) + "," + std::to_string(static_cast<int>(mousePos.y)) +
                            "  |  Sprites " + std::to_string(totalSprites) +
                            "  |  Selected " + std::to_string(selectedCount) +
                            "  |  Autosaved";
    m_statusText.setString(statusStr);
}

bool WorkspaceEnvironment::HandleEvent(const sf::Event& event, const sf::RenderWindow& window) {
    if (m_showContextMenu) {
        sf::Vector2i mousePixel = sf::Mouse::getPosition(window);
        sf::Vector2f mousePos(mousePixel.x, mousePixel.y);

        float itemHeight = 24.0f;
        float menuWidth = 140.0f;
        float totalHeight = m_menuItems.size() * itemHeight;

        sf::FloatRect menuBounds(m_menuPosition.x, m_menuPosition.y, menuWidth, totalHeight);

        if (event.type == sf::Event::MouseMoved) {
            if (menuBounds.contains(mousePos)) {
                m_hoveredMenuItem = static_cast<int>((mousePos.y - m_menuPosition.y) / itemHeight);
            } else {
                m_hoveredMenuItem = -1;
            }
            return true;
        }

        if (event.type == sf::Event::MouseButtonPressed) {
            if (menuBounds.contains(mousePos) && m_hoveredMenuItem >= 0 && m_hoveredMenuItem < static_cast<int>(m_menuItems.size())) {
                m_menuItems[m_hoveredMenuItem].action();
            }
            HideContextMenu();
            return true;
        }
    }
    return false;
}

void WorkspaceEnvironment::ShowContextMenu(sf::Vector2f position, const std::vector<ContextMenuItem>& items) {
    m_menuPosition = position;
    m_menuItems = items;
    m_showContextMenu = true;
    m_hoveredMenuItem = -1;
}

void WorkspaceEnvironment::HideContextMenu() {
    m_showContextMenu = false;
    m_menuItems.clear();
}

void WorkspaceEnvironment::Render(sf::RenderWindow& window) {
    // 1. Render Status Bar at the bottom using bounds math
    float yPos = m_bounds.top + m_bounds.height - Theme::StatusBarHeight;

    m_statusBarBg.setPosition(m_bounds.left, yPos);
    m_statusBarBg.setSize(sf::Vector2f(m_bounds.width, Theme::StatusBarHeight));

    m_statusBarTopBorder.setPosition(m_bounds.left, yPos);
    m_statusBarTopBorder.setSize(sf::Vector2f(m_bounds.width, Theme::BorderThickness));

    m_statusText.setPosition(m_bounds.left + 8.0f, yPos + 4.0f);

    window.draw(m_statusBarBg);
    window.draw(m_statusBarTopBorder);
    window.draw(m_statusText);

    // 2. Render Context Menu if active
    if (m_showContextMenu) {
        float itemHeight = 24.0f;
        float menuWidth = 140.0f;
        float totalHeight = m_menuItems.size() * itemHeight;

        sf::RectangleShape menuBg(sf::Vector2f(menuWidth, totalHeight));
        menuBg.setPosition(m_menuPosition);
        menuBg.setFillColor(Theme::InspectorBackground);
        menuBg.setOutlineThickness(Theme::BorderThickness);
        menuBg.setOutlineColor(Theme::BorderColor);
        window.draw(menuBg);

        for (size_t i = 0; i < m_menuItems.size(); ++i) {
            float itemY = m_menuPosition.y + (i * itemHeight);

            if (static_cast<int>(i) == m_hoveredMenuItem) {
                sf::RectangleShape hoverRect(sf::Vector2f(menuWidth, itemHeight));
                hoverRect.setPosition(m_menuPosition.x, itemY);
                hoverRect.setFillColor(Theme::HoverColor);
                window.draw(hoverRect);
            }

            sf::Text itemText;
            itemText.setFont(m_font);
            itemText.setString(m_menuItems[i].label);
            itemText.setCharacterSize(12);
            itemText.setFillColor(Theme::TextPrimary);
            itemText.setPosition(m_menuPosition.x + 8.0f, itemY + 4.0f);
            window.draw(itemText);
        }
    }
}
} // namespace StudioUI