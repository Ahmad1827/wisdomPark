#pragma once
#include "../core/Canvas.h"
#include "../core/Timeline.h"
#include "../core/ITool.h"
#include "../core/TextSystem.h"

class TextTool : public ITool {
public:
    TextTool(Canvas& canvas, Timeline& timeline, TextManager& tm);
    ~TextTool() override;
    void Initialize() override;
    void HandleEvent(const sf::Event& event, const sf::RenderWindow& window) override;
    void Update(float deltaTime, const sf::RenderWindow& window) override;
    void Render(sf::RenderWindow& window) override;
    void SetBounds(const sf::FloatRect& bounds) override;
    void commitText();

private:
    Canvas& m_canvas;
    Timeline& m_timeline;
    TextManager& m_tm;
    sf::FloatRect m_bounds;
    bool m_isPanning;
    sf::Vector2f m_lastPanPos;
    sf::Color m_lastPrimaryColor; // Tracks the global color for live updates
    bool m_isDraggingText{ false };
    sf::Vector2f m_textDragStartMouse{ 0.f, 0.f };
    sf::Vector2f m_textDragStartPos{ 0.f, 0.f };
};