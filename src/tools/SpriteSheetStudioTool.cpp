#include "SpriteSheetStudioTool.h"

void SpriteSheetStudioTool::Initialize() {
    m_panel.Initialize();
}

void SpriteSheetStudioTool::HandleEvent(const sf::Event& event, const sf::RenderWindow& window) {
    // Bypass the inheritance bug by forcing the panel to sync with the true window size
    sf::FloatRect physicalSpace(0.f, 0.f, static_cast<float>(window.getSize().x), static_cast<float>(window.getSize().y));
    m_panel.SetBounds(physicalSpace);

    m_panel.HandleEvent(event, window);
}

void SpriteSheetStudioTool::Update(float deltaTime, const sf::RenderWindow& window) {
    // Continuously force the standalone panel to adopt the true physical window size
    sf::FloatRect physicalSpace(0.f, 0.f, static_cast<float>(window.getSize().x), static_cast<float>(window.getSize().y));
    m_panel.SetBounds(physicalSpace);

    m_panel.Update(deltaTime, window);
}

void SpriteSheetStudioTool::Render(sf::RenderWindow& window) {
    sf::FloatRect physicalSpace(0.f, 0.f, static_cast<float>(window.getSize().x), static_cast<float>(window.getSize().y));
    m_panel.SetBounds(physicalSpace);

    m_panel.Render(window);
}

void SpriteSheetStudioTool::SetBounds(const sf::FloatRect& bounds) {
    m_bounds = bounds;
    m_panel.SetBounds(bounds);
}