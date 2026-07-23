#pragma once
#include <SFML/Graphics.hpp>

namespace StudioUI {

class CoordinateAxisRenderer {
public:
    CoordinateAxisRenderer();
    void Render(sf::RenderWindow& window, const sf::FloatRect& imageBounds, float currentZoom);

private:
    sf::Color m_xAxisColor{255, 50, 50, 200};
    sf::Color m_yAxisColor{50, 255, 50, 200};
    sf::Color m_rulerColor{200, 200, 200, 150};
};

}