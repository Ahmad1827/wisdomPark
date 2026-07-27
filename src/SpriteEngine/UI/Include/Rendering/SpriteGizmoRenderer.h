#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include <string>
#include <vector>

namespace StudioCore {
    class SpriteDefinition;
}

namespace StudioUI {

class SpriteGizmoRenderer {
public:
    SpriteGizmoRenderer();
    
    void SetHoveredSprite(const std::string& id);
    void SetSelectedSprites(const std::vector<std::string>& ids);
    
    void Render(sf::RenderWindow& window, 
                const std::vector<std::shared_ptr<StudioCore::SpriteDefinition>>& sprites, 
                float currentZoom,
                bool showBoxes,
                bool showCenters,
                bool showPivots,
                bool showBaselines,
                bool showIds, 
                sf::Font* font);

private:
    std::string m_hoveredId;
    std::vector<std::string> m_selectedIds;
    
    sf::RectangleShape m_box;
    sf::CircleShape m_centerDot;
    sf::CircleShape m_pivotDot;
    sf::RectangleShape m_baselineLine;
};

} // namespace StudioUI