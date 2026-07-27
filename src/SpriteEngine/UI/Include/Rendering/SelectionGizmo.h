#pragma once
#include <SFML/Graphics.hpp>

namespace StudioUI {

class SelectionGizmo {
public:
    SelectionGizmo();

    void BeginSelection(const sf::Vector2f& startPos);
    void UpdateSelection(const sf::Vector2f& currentPos);
    void EndSelection();
    void ClearSelection();

    bool HasValidSelection() const;
    sf::FloatRect GetSelectionRect() const;

    void Render(sf::RenderWindow& window, float currentZoom);

private:
    bool m_isSelecting{false};
    bool m_hasFinishedSelection{false};
    sf::Vector2f m_startPos;
    sf::Vector2f m_endPos;
    sf::RectangleShape m_rectShape;
};

}