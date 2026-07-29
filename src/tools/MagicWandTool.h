#pragma once
#include "../core/Canvas.h"
#include "../core/Timeline.h"
#include "../core/ITool.h"
#include <vector>
#include <queue>

class MagicWandTool : public ITool {
public:
    MagicWandTool(Canvas& canvas, Timeline& timeline);
    void Initialize() override;
    void HandleEvent(const sf::Event& event, const sf::RenderWindow& window) override;
    void Update(float deltaTime, const sf::RenderWindow& window) override;
    void Render(sf::RenderWindow& window) override;
    void SetBounds(const sf::FloatRect& bounds) override;

private:
    Canvas& m_canvas;
    Timeline& m_timeline;
    sf::FloatRect m_bounds;

    sf::RectangleShape m_panelBg;
    sf::Font m_font;

    int m_tolerance;
    bool m_contiguous;
    bool m_sampleAllLayers;

    bool m_isPanning;
    sf::Vector2f m_lastPanPos;

    float getPerceptualDistance(sf::Color c1, sf::Color c2);
    std::vector<bool> extractSelectionMask(sf::Vector2i startPos);
    std::vector<sf::Vector2f> traceBoundary(const std::vector<bool>& mask, int w, int h);

    void drawPropertiesPanel(sf::RenderWindow& window);
};