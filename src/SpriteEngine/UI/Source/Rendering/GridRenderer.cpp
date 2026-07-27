#include "Rendering/GridRenderer.h"
#include "Theme.h"

namespace StudioUI {

GridRenderer::GridRenderer() = default;

void GridRenderer::Render(sf::RenderWindow& window, const sf::FloatRect& bounds) {
    float gridSize = 16.0f;

    int startX = static_cast<int>(bounds.left / gridSize) - 1;
    int endX = static_cast<int>((bounds.left + bounds.width) / gridSize) + 1;
    int startY = static_cast<int>(bounds.top / gridSize) - 1;
    int endY = static_cast<int>((bounds.top + bounds.height) / gridSize) + 1;

    sf::RectangleShape cell({gridSize, gridSize});

    for (int y = startY; y < endY; ++y) {
        for (int x = startX; x < endX; ++x) {
            bool isDark = ((x + y) % 2 == 0);
            
            // Subtle dark checkerboard matching the Viewport background
            cell.setFillColor(isDark ? Theme::ViewportBackground : sf::Color(32, 34, 38));
            cell.setPosition(x * gridSize, y * gridSize);
            window.draw(cell);
        }
    }
}

} // namespace StudioUI