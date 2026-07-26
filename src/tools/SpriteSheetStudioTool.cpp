#include "SpriteSheetStudioTool.h"

void SpriteSheetStudioTool::Initialize() {
    m_panel.Initialize();
}

void SpriteSheetStudioTool::HandleEvent(const sf::Event& event, const sf::RenderWindow& window) {
    if (event.type == sf::Event::Resized) {
        sf::FloatRect newBounds(0.f, 0.f, static_cast<float>(event.size.width), static_cast<float>(event.size.height));
        SetBounds(newBounds);
    }
    m_panel.HandleEvent(event, window);
}

void SpriteSheetStudioTool::Update(float deltaTime, const sf::RenderWindow& window) {
    sf::Vector2u winSize = window.getSize();
    sf::FloatRect currentBounds(0.f, 0.f, static_cast<float>(winSize.x), static_cast<float>(winSize.y));
    SetBounds(currentBounds);

    m_panel.Update(deltaTime, window);
}

void SpriteSheetStudioTool::Render(sf::RenderWindow& window) {
    sf::Vector2u winSize = window.getSize();
    sf::View uiView(sf::FloatRect(0.f, 0.f, static_cast<float>(winSize.x), static_cast<float>(winSize.y)));
    window.setView(uiView);

    sf::RectangleShape darkBg(sf::Vector2f(static_cast<float>(winSize.x), static_cast<float>(winSize.y)));
    darkBg.setFillColor(sf::Color(18, 18, 22));
    window.draw(darkBg);

    m_panel.Render(window);
}

void SpriteSheetStudioTool::SetBounds(const sf::FloatRect& bounds) {
    m_bounds = bounds;
    m_panel.SetBounds(bounds);
}