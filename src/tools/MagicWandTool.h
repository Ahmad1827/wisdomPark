#pragma once
#include "../core/Canvas.h"
#include "../core/Timeline.h"
#include "../core/ITool.h"
#include <vector>
#include <queue>
#include <map>

enum class SelectionBlendMode { Replace, Add, Subtract, Intersect };

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
    SelectionBlendMode m_blendMode;
    bool m_contiguous;
    bool m_sampleAllLayers;
    bool m_antiAlias;
    int m_feather;

    sf::Vector2i m_lastHoverPos;
    std::vector<std::vector<sf::Vector2f>> m_previewContours;

    float getPerceptualDistance(sf::Color c1, sf::Color c2);
    void growMask(std::vector<bool>& mask, int w, int h, int amount);
    void shrinkMask(std::vector<bool>& mask, int w, int h, int amount);
    std::vector<bool> extractSelectionMask(sf::Vector2i startPos);
    std::vector<std::vector<sf::Vector2f>> generateContours(const std::vector<bool>& mask, int w, int h);
    void smoothContours(std::vector<std::vector<sf::Vector2f>>& contours);
    std::vector<sf::Vector2f> bridgeContours(const std::vector<std::vector<sf::Vector2f>>& contours);

    void applyWandSelection(const std::vector<bool>& newMask);
    void updatePreview(sf::Vector2i pos);
    void drawPropertiesPanel(sf::RenderWindow& window);

    bool m_isPanning;
    sf::Vector2f m_lastPanPos;
};