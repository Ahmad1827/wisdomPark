#pragma once
#include "../core/ITool.h"
#include "../core/Canvas.h"
#include "../core/Timeline.h"
#include "../core/GradientSystem.h"

class GradientTool : public ITool {
public:
    GradientTool(Canvas& canvas, Timeline& timeline, GradientConfig& config);
    void Initialize() override;
    void HandleEvent(const sf::Event& event, const sf::RenderWindow& window) override;
    void Update(float deltaTime, const sf::RenderWindow& window) override;
    void Render(sf::RenderWindow& window) override;
    void SetBounds(const sf::FloatRect& bounds) override;

private:
    Canvas& m_canvas;
    Timeline& m_timeline;
    GradientConfig& m_config;
    sf::FloatRect m_bounds;
    bool m_isPanning;
    bool m_isDragging;
    sf::Vector2f m_lastPanPos;
    sf::Vector2f m_startPos;
    sf::Vector2f m_currentPos;
    sf::Texture m_previewTexture;
    sf::Sprite m_previewSprite;

    sf::Vector2f applyModifiers(sf::Vector2f rawPos);
    void applyGradient();
    void updatePreview();
};