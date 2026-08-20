#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include "../core/Canvas.h"
#include "../core/Timeline.h"
#include "../core/ITool.h"
#include "../core/ShapeSystem.h"

class ShapeTool : public ITool {
public:
    ShapeTool(Canvas& canvas, Timeline& timeline);
    void Initialize() override;
    void HandleEvent(const sf::Event& event, const sf::RenderWindow& window) override;
    void Update(float deltaTime, const sf::RenderWindow& window) override;
    void Render(sf::RenderWindow& window) override;
    void SetBounds(const sf::FloatRect& bounds) override;

private:
    Canvas& m_canvas;
    Timeline& m_timeline;
    sf::FloatRect m_bounds;
    ShapeManager m_shapeManager;
    ShapeId m_currentShapeId;

    bool m_isDragging;
    bool m_isPanning;
    sf::Vector2f m_lastPanPos;

    sf::Vector2f m_startPos;
    sf::Font m_font;

    sf::Vector2f m_panelPos;
    sf::Vector2f m_panelSize;
    bool m_isDraggingPanel;
    sf::Vector2f m_panelDragOffset;

    std::vector<std::pair<sf::FloatRect, ShapeId>> m_shapeButtons;

    void drawPropertiesPanel(sf::RenderWindow& window);
};