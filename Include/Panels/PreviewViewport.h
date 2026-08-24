#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include <string>
#include <vector>
#include "Rendering/GridRenderer.h"
#include "Rendering/SelectionGizmo.h"
#include "Rendering/OverlayRenderer.h"
#include "Rendering/CoordinateAxisRenderer.h"
#include "Rendering/SpriteGizmoRenderer.h"
#include <unordered_map>
#include "DataModels/SpriteDefinition.h"
#include "DataModels/SourceTexture.h"

namespace StudioCore {
    class StudioEngineFacade;
}

class PreviewViewport {
public:
    PreviewViewport();
    void Initialize();
    const std::vector<std::string>& GetSelectedSpriteIds() const { return m_selectedSpriteIds; }
    void HandleEvent(const sf::Event& event, const sf::RenderWindow& window, StudioCore::StudioEngineFacade& engine);
    void Update(float deltaTime);
    void Render(sf::RenderWindow& window, const StudioCore::StudioEngineFacade& engine);
    void RemoveCustomSpriteTexture(const std::string& spriteId);
    void ClearSelection() { 
        m_selectedSpriteIds.clear(); 
        m_selection.ClearSelection();
    }
    sf::Vector2f MapPixelToWorld(const sf::Vector2i& pixelPos, const sf::RenderWindow& window) const {
        return window.mapPixelToCoords(pixelPos, m_view); 
    }
    void SetUIHidden(bool hidden) { m_isUIHidden = hidden; }
    
    // Restored Dynamic Bounds to prevent Timeline click hijacking
    sf::FloatRect GetViewportBounds(const sf::RenderWindow& window) const {
        if (m_bounds.width > 0.0f && m_bounds.height > 0.0f) {
            return m_bounds;
        }
        // Fallback for standalone safety
        sf::Vector2u size = window.getSize();
        float startY = 40.f; 
        float rightPad = m_isUIHidden ? 0.f : 300.f;
        float bottomPad = m_isUIHidden ? 0.f : 200.f;
        return sf::FloatRect(0.f, startY, static_cast<float>(size.x) - rightPad, static_cast<float>(size.y) - bottomPad - startY);
    }
    
    void RefreshTexture(const StudioCore::StudioEngineFacade& engine);
    void ResetZoom();
    void ResetPan();
    void CenterImage();
    void FitToImage(const sf::RenderWindow& window);
    void ZoomToSelection();
    void CenterOnPoint(const sf::Vector2f& worldPos);
    void TriggerNumericEdit(StudioCore::StudioEngineFacade& engine);
    void SetBounds(const sf::FloatRect& bounds) { m_bounds = bounds; }

private:
    sf::FloatRect m_bounds{0.0f, 0.0f, 1280.0f, 720.0f};
    void RenderSprite(sf::RenderTarget& target, const StudioCore::SpriteDefinition& sprite, const StudioCore::SourceTexture& sourceTex);
    sf::Vector2i GetMouseImageCoords(const sf::RenderWindow& window) const;
    sf::Color GetPixelColor(const StudioCore::StudioEngineFacade& engine, int x, int y) const;
    void HandleSpriteSelection(const sf::Vector2f& worldPos, const StudioCore::StudioEngineFacade& engine, bool shift, bool ctrl);

    sf::Texture m_gpuTexture;
    sf::Sprite m_sprite;
    sf::View m_view;
    sf::Texture m_sourceTextureGL;
    bool m_hasValidTexture{false};
    float m_currentZoom{1.0f};
    bool m_isUIHidden{false};
    bool m_isPanning{false};
    bool m_isDraggingPivot{false};
    bool m_isDraggingBaseline{false};
    sf::Vector2i m_lastMousePos;
    sf::Clock m_doubleClickTimer;

    bool m_showGrid{false};
    bool m_showDebug{false};
    bool m_showBoxes{true};
    bool m_showCenters{true};
    bool m_showIds{true};
    bool m_showPivots{true};
    bool m_showBaselines{true};

    std::vector<std::string> m_selectedSpriteIds;
    std::string m_hoveredSpriteId;

    float m_fps{0.0f};
    float m_frameTimeMs{0.0f};
    int m_frameCount{0};
    float m_fpsTimer{0.0f};

    StudioUI::GridRenderer m_grid;
    StudioUI::SelectionGizmo m_selection;
    StudioUI::OverlayRenderer m_overlay;
    StudioUI::CoordinateAxisRenderer m_axes;
    StudioUI::SpriteGizmoRenderer m_spriteGizmos;
};