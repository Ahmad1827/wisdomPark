#include "Rendering/SelectionGizmo.h"
#include <cmath>
#include <algorithm>

namespace StudioUI {

SelectionGizmo::SelectionGizmo() {
    m_rectShape.setFillColor(sf::Color(0, 255, 255, 50)); // Semi-transparent Cyan
    m_rectShape.setOutlineColor(sf::Color::Cyan);
}

void SelectionGizmo::BeginSelection(const sf::Vector2f& startPos) {
    m_isSelecting = true;
    m_hasFinishedSelection = false;
    m_startPos = startPos;
    m_endPos = startPos;
}

void SelectionGizmo::UpdateSelection(const sf::Vector2f& currentPos) {
    if (m_isSelecting) {
        m_endPos = currentPos;
    }
}

void SelectionGizmo::EndSelection() {
    m_isSelecting = false;
    if (std::abs(m_endPos.x - m_startPos.x) > 1.0f && std::abs(m_endPos.y - m_startPos.y) > 1.0f) {
        m_hasFinishedSelection = true;
    } else {
        m_hasFinishedSelection = false; // Ignore tiny accidental clicks
    }
}

void SelectionGizmo::ClearSelection() {
    m_isSelecting = false;
    m_hasFinishedSelection = false;
}

bool SelectionGizmo::HasValidSelection() const { 
    return m_isSelecting || m_hasFinishedSelection; 
}

sf::FloatRect SelectionGizmo::GetSelectionRect() const {
    float left = std::min(m_startPos.x, m_endPos.x);
    float top = std::min(m_startPos.y, m_endPos.y);
    float width = std::abs(m_endPos.x - m_startPos.x);
    float height = std::abs(m_endPos.y - m_startPos.y);
    return sf::FloatRect(left, top, width, height);
}

void SelectionGizmo::Render(sf::RenderWindow& window, float currentZoom) {
    if (!HasValidSelection()) return;

    sf::FloatRect rect = GetSelectionRect();
    m_rectShape.setPosition(rect.left, rect.top);
    m_rectShape.setSize(sf::Vector2f(rect.width, rect.height));
    
    // Constant 2px visual outline regardless of zoom
    m_rectShape.setOutlineThickness(2.0f * currentZoom);

    window.draw(m_rectShape);
}

}