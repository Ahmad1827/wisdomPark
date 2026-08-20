#pragma once
#include <SFML/Graphics.hpp>
#include "../core/PerspectiveSystem.h"
#include <string>

class PerspectivePanel {
private:
    PerspectiveManager* m_pm;
    sf::Font m_font;
    sf::Vector2f m_position;
    sf::Vector2f m_size;

    bool m_isDraggingPanel = false;
    sf::Vector2f m_dragOffset;
    bool m_presetsOpen;

    void drawButton(sf::RenderWindow& window, sf::FloatRect bounds, const std::string& text, sf::Color bgColor);
    void drawToggle(sf::RenderWindow& window, sf::FloatRect bounds, const std::string& text, bool state);

public:
    PerspectivePanel();
    void init(PerspectiveManager* pm);
    void update(float dt);
    void draw(sf::RenderWindow& window);
    void handleEvent(const sf::Event& event, sf::Vector2f mousePos, sf::Vector2u canvasSize);
};