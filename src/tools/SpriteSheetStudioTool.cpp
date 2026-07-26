#include "SpriteSheetStudioTool.h"

void SpriteSheetStudioTool::Initialize() {
    m_panel.Initialize();
}

void SpriteSheetStudioTool::HandleEvent(const sf::Event& event, const sf::RenderWindow& window) {
    sf::FloatRect physicalSpace(0.f, 0.f, static_cast<float>(window.getSize().x), static_cast<float>(window.getSize().y));
    SetBounds(physicalSpace);
    m_panel.HandleEvent(event, window);
}

void SpriteSheetStudioTool::Update(float deltaTime, const sf::RenderWindow& window) {
    sf::FloatRect physicalSpace(0.f, 0.f, static_cast<float>(window.getSize().x), static_cast<float>(window.getSize().y));
    SetBounds(physicalSpace);
    m_panel.Update(deltaTime, window);
}

void SpriteSheetStudioTool::Render(sf::RenderWindow& window) {
    sf::FloatRect physicalSpace(0.f, 0.f, static_cast<float>(window.getSize().x), static_cast<float>(window.getSize().y));
    SetBounds(physicalSpace);

    // Remember whatever view Wisdom Park had active before we take over, so
    // we can hand it back afterward. Nothing else in the app resets the view
    // itself, so leaving ours active corrupts every subsequent draw call
    // this frame and every frame after, until something else happens to
    // reset it.
    sf::View callerView = window.getView();

    sf::View absoluteView(physicalSpace);
    absoluteView.setViewport(sf::FloatRect(0.f, 0.f, 1.f, 1.f));
    window.setView(absoluteView);

    sf::RectangleShape bg(sf::Vector2f(physicalSpace.width, physicalSpace.height));
    bg.setFillColor(sf::Color(18, 18, 22));
    window.draw(bg);

    m_panel.Render(window);

    // Hand the view back so Wisdom Park's own fixed-coordinate UI keeps
    // drawing correctly once this tool is done for the frame.
    window.setView(callerView);
}
void SpriteSheetStudioTool::SetBounds(const sf::FloatRect& bounds) {
    m_bounds = bounds;
    m_panel.SetBounds(bounds);
}