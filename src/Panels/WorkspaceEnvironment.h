#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <functional>

namespace StudioUI {

struct ContextMenuItem {
    std::string label;
    std::function<void()> action;
};

class WorkspaceEnvironment {
public:
    WorkspaceEnvironment() = default;

    void InitializeFont(const std::string& fontPath);
    bool HandleEvent(const sf::Event& event, const sf::RenderWindow& window);
    void Render(sf::RenderWindow& window);
    void UpdateStatusBar(float zoom, sf::Vector2f mousePos, int totalSprites, int selectedCount, const std::string& statusText);
    void SetBounds(const sf::FloatRect& bounds) { m_bounds = bounds; }
    void ShowContextMenu(sf::Vector2f position, const std::vector<ContextMenuItem>& items);
    void HideContextMenu();

private:
    sf::FloatRect m_bounds{0.0f, 0.0f, 1280.0f, 720.0f};
    sf::Font m_font;
    sf::RectangleShape m_statusBarBg;
    sf::RectangleShape m_statusBarTopBorder;
    sf::Text m_statusText;

    // Context Menu UI
    bool m_showContextMenu{false};
    sf::Vector2f m_menuPosition;
    std::vector<ContextMenuItem> m_menuItems;
    int m_hoveredMenuItem{-1};
};

} // namespace StudioUI