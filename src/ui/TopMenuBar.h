#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

struct TopMenuButton {
    std::string id;
    std::string label;
    sf::FloatRect bounds;
    bool isHovered = false;
};

class TopMenuBar {
public:
    TopMenuBar();
    void init(float windowWidth);
    void update(sf::Vector2f mousePos);
    void draw(sf::RenderWindow& window);
    std::string handleEvent(const sf::Event& event, sf::Vector2f mousePos);
    void resize(float windowWidth);

private:
    sf::RectangleShape m_background;
    sf::CircleShape m_leftCap;
    sf::CircleShape m_rightCap;
    sf::Font m_font;
    std::vector<TopMenuButton> m_buttons;
};