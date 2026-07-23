#pragma once
#include "../core/ITool.h"
#include "../core/Canvas.h"
#include "../core/Timeline.h"
#include "../ai/AIHelper.h"

class CanvasTool : public ITool {
private:
    Canvas& m_canvas;
    Timeline& m_timeline;
    bool& m_isLightingMode;

    sf::FloatRect m_bounds;
    bool m_isPanning;
    sf::Vector2f m_lastPanMousePos;

public:
    CanvasTool(Canvas& canvas, Timeline& timeline, bool& isLightingMode);

    void Initialize() override;
    void HandleEvent(const sf::Event& event, const sf::RenderWindow& window) override;
    void Update(float deltaTime, const sf::RenderWindow& window) override;
    void Render(sf::RenderWindow& window) override;
    void SetBounds(const sf::FloatRect& bounds) override;

    // Custom helper for native canvas shadow rendering
    void RenderShadows(sf::RenderWindow& window, AIHelper& aiHelper);
};