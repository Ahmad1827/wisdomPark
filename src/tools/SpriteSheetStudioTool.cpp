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
    // Reset view to physical window dimensions to cover Wisdom Park background
    window.setView(window.getDefaultView());

    sf::RectangleShape darkBg(sf::Vector2f(static_cast<float>(window.getSize().x), static_cast<float>(window.getSize().y)));
    darkBg.setFillColor(sf::Color(18, 18, 22));
    window.draw(darkBg);

    // Render Sprite Sheet Studio UI on top
    m_panel.Render(window);
}

void SpriteSheetStudioTool::SetBounds(const sf::FloatRect& bounds) {
    m_bounds = bounds;
    m_panel.SetBounds(bounds);
}