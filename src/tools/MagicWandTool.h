#pragma once
#include <SFML/Graphics.hpp>
#include <queue>
#include <vector>
#include "../core/Canvas.h"
#include "../core/Timeline.h"
#include "../core/ITool.h"

class MagicWandTool : public ITool {
public:
    MagicWandTool(Canvas& canvas, Timeline& timeline);
    void Initialize() override;
    void HandleEvent(const sf::Event& event, const sf::RenderWindow& window) override;
    void Update(float deltaTime, const sf::RenderWindow& window) override;
    void Render(sf::RenderWindow& window) override;
    void SetBounds(const sf::FloatRect& bounds) override;

    bool wantsColorPanelOpen() const;
    void clearColorPanelRequest();

private:
    Canvas& m_canvas;
    Timeline& m_timeline;
    sf::FloatRect m_bounds;

    int m_tolerance;
    bool m_contiguous;
    bool m_sampleAllLayers;

    bool m_isPanning;
    sf::Vector2f m_lastPanPos;

    sf::Font m_font;
    sf::Vector2f m_panelPos;
    sf::Vector2f m_panelSize;
    bool m_isDraggingPanel;
    sf::Vector2f m_panelDragOffset;

    sf::FloatRect m_colorBoxRect;
    sf::FloatRect m_tolMinusRect;
    sf::FloatRect m_tolPlusRect;
    sf::FloatRect m_contigRect;
    sf::FloatRect m_sampleRect;

    bool m_requestColorPanelOpen;
    sf::Color m_lastPrimaryColor;

    float getPerceptualDistance(sf::Color c1, sf::Color c2);
    std::vector<bool> extractSelectionMask(sf::Vector2i startPos);
    std::vector<sf::Vector2f> traceBoundary(const std::vector<bool>& mask, int w, int h, sf::Vector2i startNode);
    void drawPropertiesPanel(sf::RenderWindow& window);
};