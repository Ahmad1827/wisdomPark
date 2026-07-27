#include "Rendering/SpriteGizmoRenderer.h"
#include "DataModels/SpriteDefinition.h"
#include <algorithm>

namespace StudioUI {

SpriteGizmoRenderer::SpriteGizmoRenderer() {
    m_box.setFillColor(sf::Color::Transparent);
    
    m_centerDot.setFillColor(sf::Color(255, 50, 50, 255));
    m_centerDot.setOutlineColor(sf::Color::White);
    
    m_pivotDot.setFillColor(sf::Color(50, 255, 50, 255));
    m_pivotDot.setOutlineColor(sf::Color::Black);

    m_baselineLine.setFillColor(sf::Color(255, 170, 0, 230));
}

void SpriteGizmoRenderer::SetHoveredSprite(const std::string& id) {
    m_hoveredId = id;
}

void SpriteGizmoRenderer::SetSelectedSprites(const std::vector<std::string>& ids) {
    m_selectedIds = ids;
}

void SpriteGizmoRenderer::Render(sf::RenderWindow& window, 
                                 const std::vector<std::shared_ptr<StudioCore::SpriteDefinition>>& sprites, 
                                 float currentZoom,
                                 bool showBoxes,
                                 bool showCenters,
                                 bool showPivots,
                                 bool showBaselines,
                                 bool showIds, 
                                 sf::Font* font) {
    
    float scaleFactor = (currentZoom > 0.0001f) ? currentZoom : 1.0f;
    float boxThickness = 3.0f * scaleFactor;
    float selectedThickness = 5.0f * scaleFactor;
    float hoverThickness = 4.0f * scaleFactor;

    for (const auto& sprite : sprites) {
        if (!sprite) continue;

        const auto& rect = sprite->GetSourceRect();
        bool isHovered = (sprite->GetId() == m_hoveredId);
        bool isSelected = std::find(m_selectedIds.begin(), m_selectedIds.end(), sprite->GetId()) != m_selectedIds.end();
        bool isMultiSelect = (m_selectedIds.size() > 1);

        // 1. Sprite Bounding Boxes & Outlines (Bright & Thick)
        if (showBoxes || isHovered || isSelected) {
            m_box.setPosition(static_cast<float>(rect.x), static_cast<float>(rect.y));
            m_box.setSize(sf::Vector2f(static_cast<float>(rect.width), static_cast<float>(rect.height)));
            
            if (isSelected) {
                m_box.setOutlineColor(isMultiSelect ? sf::Color(255, 120, 0, 255) : sf::Color(255, 235, 0, 255));
                m_box.setOutlineThickness(selectedThickness);
            } else if (isHovered) {
                m_box.setOutlineColor(sf::Color(0, 240, 255, 255));
                m_box.setOutlineThickness(hoverThickness);
            } else {
                m_box.setOutlineColor(sf::Color(255, 0, 220, 255));
                m_box.setOutlineThickness(boxThickness);
            }
            window.draw(m_box);
        }

        // 2. Sprite Geometric Center
        if (showCenters) {
            const auto& c = sprite->GetCenter();
            float radius = 4.0f * scaleFactor;
            m_centerDot.setRadius(radius);
            m_centerDot.setOutlineThickness(1.5f * scaleFactor);
            m_centerDot.setOrigin(radius, radius);
            m_centerDot.setPosition(c.x, c.y);
            window.draw(m_centerDot);
        }

        // 3. Pivot Dot Indicator
        if (showPivots || isSelected) {
            const auto& p = sprite->GetPivot();
            float px = rect.x + (p.x * rect.width);
            float py = rect.y + (p.y * rect.height);

            float radius = 5.0f * scaleFactor;
            m_pivotDot.setRadius(radius);
            m_pivotDot.setOutlineThickness(1.5f * scaleFactor);
            m_pivotDot.setOrigin(radius, radius);
            m_pivotDot.setPosition(px, py);
            window.draw(m_pivotDot);
        }

        // 4. Baseline Line Marker
        if (showBaselines || isSelected) {
            float by = rect.y + rect.height - sprite->GetBaseline();
            float lineH = 2.5f * scaleFactor;
            m_baselineLine.setPosition(static_cast<float>(rect.x), by);
            m_baselineLine.setSize(sf::Vector2f(static_cast<float>(rect.width), lineH));
            window.draw(m_baselineLine);
        }

        // 5. Sprite ID Text Overlay
        if (showIds && font) {
            unsigned int fontSize = static_cast<unsigned int>(std::max(12.0f, 14.0f * scaleFactor));
            sf::Text idText(sprite->GetId(), *font, fontSize);
            idText.setPosition(static_cast<float>(rect.x), rect.y - (fontSize + 4.0f * scaleFactor));
            idText.setFillColor(isSelected ? sf::Color(255, 235, 0, 255) : sf::Color(255, 255, 255, 255));
            
            sf::Text idShadow = idText;
            idShadow.setPosition(idText.getPosition().x + (1.5f * scaleFactor), idText.getPosition().y + (1.5f * scaleFactor));
            idShadow.setFillColor(sf::Color(0, 0, 0, 255));

            window.draw(idShadow);
            window.draw(idText);
        }
    }
}

} // namespace StudioUI