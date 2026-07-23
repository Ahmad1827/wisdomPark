#include "SpriteSheetStudioTool.h"

void SpriteSheetStudioTool::Initialize() {
    m_panel.Initialize();
}

void SpriteSheetStudioTool::HandleEvent(const sf::Event& event, const sf::RenderWindow& window) {
    m_panel.HandleEvent(event, window);
}

void SpriteSheetStudioTool::Update(float deltaTime, const sf::RenderWindow& window) {
    m_panel.Update(deltaTime, window);
}

void SpriteSheetStudioTool::Render(sf::RenderWindow& window) {
    window.setView(window.getDefaultView());

    // Fill background strictly within the designated bounds
    sf::RectangleShape panelBg(sf::Vector2f(m_bounds.width, m_bounds.height));
    panelBg.setPosition(m_bounds.left, m_bounds.top);
    panelBg.setFillColor(sf::Color(18, 18, 22));
    window.draw(panelBg);

    m_panel.Render(window);
}

void SpriteSheetStudioTool::SetBounds(const sf::FloatRect& bounds) {
    m_bounds = bounds;
    m_panel.SetBounds(bounds);
}