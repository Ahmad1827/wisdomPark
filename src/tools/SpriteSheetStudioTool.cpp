#include "SpriteSheetStudioTool.h"

void SpriteSheetStudioTool::Initialize() {
    m_panel.Initialize();
}

void SpriteSheetStudioTool::HandleEvent(const sf::Event& event, const sf::RenderWindow& window) {
    sf::Event mappedEvent = event;

    // Force the internal tool UI to always map to the 1920x1080 view
    if (event.type == sf::Event::Resized) {
        mappedEvent.size.width = 1920;
        mappedEvent.size.height = 1080;
    }
    // Convert physical mouse pixels to logical 1920x1080 pixels
    else if (event.type == sf::Event::MouseMoved) {
        sf::Vector2i pixelPos(event.mouseMove.x, event.mouseMove.y);
        sf::Vector2f logicalPos = window.mapPixelToCoords(pixelPos, sf::View(sf::FloatRect(0.f, 0.f, 1920.f, 1080.f)));
        mappedEvent.mouseMove.x = static_cast<int>(logicalPos.x);
        mappedEvent.mouseMove.y = static_cast<int>(logicalPos.y);
    }
    else if (event.type == sf::Event::MouseButtonPressed || event.type == sf::Event::MouseButtonReleased) {
        sf::Vector2i pixelPos(event.mouseButton.x, event.mouseButton.y);
        sf::Vector2f logicalPos = window.mapPixelToCoords(pixelPos, sf::View(sf::FloatRect(0.f, 0.f, 1920.f, 1080.f)));
        mappedEvent.mouseButton.x = static_cast<int>(logicalPos.x);
        mappedEvent.mouseButton.y = static_cast<int>(logicalPos.y);
    }

    m_panel.HandleEvent(mappedEvent, window);
}

void SpriteSheetStudioTool::Update(float deltaTime, const sf::RenderWindow& window) {
    m_panel.Update(deltaTime, window);
}

void SpriteSheetStudioTool::Render(sf::RenderWindow& window) {
    window.setView(sf::View(sf::FloatRect(0.f, 0.f, 1920.f, 1080.f)));

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