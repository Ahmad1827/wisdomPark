#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

class AIPanel {
private:
    sf::Font font;
    sf::Vector2f position;
    sf::Vector2f size;

    bool isVisible;
    bool isDraggingPanel;
    sf::Vector2f dragOffset;

    sf::FloatRect headerGripBounds;
    sf::FloatRect suggestBtnBounds;
    sf::FloatRect closeBtnBounds;

public:
    AIPanel();
    void init();
    void update(float dt);
    void draw(sf::RenderWindow& window);

    bool handleEvent(const sf::Event& event, sf::Vector2f mousePos);
    std::string handleClick(sf::Vector2f mousePos);

    void toggle();
    bool getIsVisible() const;

    static std::vector<sf::Color> createColorAdvice(sf::Color baseColor, int variation = 0);
};