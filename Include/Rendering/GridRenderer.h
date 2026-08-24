#pragma once
#include <SFML/Graphics.hpp>

namespace StudioUI {

class GridRenderer {
public:
    GridRenderer();

    void SetCellSize(int width, int height);
    void Render(sf::RenderWindow& window, const sf::FloatRect& imageBounds);

private:
    int m_cellWidth{32};
    int m_cellHeight{32};
    sf::Color m_gridColor{255, 255, 255, 50};
};

}