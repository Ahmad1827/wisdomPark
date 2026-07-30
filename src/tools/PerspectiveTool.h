#pragma once
#include "../core/Canvas.h"
#include "../core/Timeline.h"
#include "../core/ITool.h"
#include "../core/PerspectiveSystem.h"

class PerspectiveTool : public ITool {
public:
    PerspectiveTool(Canvas& canvas, Timeline& timeline, PerspectiveManager& pm);
    void Initialize() override;
    void HandleEvent(const sf::Event& event, const sf::RenderWindow& window) override;
    void Update(float deltaTime, const sf::RenderWindow& window) override;
    void Render(sf::RenderWindow& window) override;
    void SetBounds(const sf::FloatRect& bounds) override;

private:
    Canvas& m_canvas;
    Timeline& m_timeline;
    PerspectiveManager& m_pm;
    sf::FloatRect m_bounds;

    bool m_isPanning;
    sf::Vector2f m_lastPanPos;

    int m_hoveredVPIndex;
    int m_draggedVPIndex;
};