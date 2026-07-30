#pragma once
#include <SFML/Graphics.hpp>
#include "../core/PerspectiveSystem.h"

class PerspectivePanel {
public:
    PerspectivePanel();
    void init(PerspectiveManager* pm);
    void update(float dt);
    void draw(sf::RenderWindow& window);
    void handleEvent(const sf::Event& event, sf::Vector2f mousePos, sf::Vector2u canvasSize);

private:
    PerspectiveManager* m_pm;
    sf::RectangleShape m_background;
    sf::Font m_font;
    bool m_presetsOpen;

    void drawButton(sf::RenderWindow& window, sf::FloatRect bounds, const std::string& text, sf::Color bgColor);
    void drawToggle(sf::RenderWindow& window, sf::FloatRect bounds, const std::string& text, bool state);
};