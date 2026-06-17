#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

struct WelcomeButton {
    sf::RectangleShape rect;
    sf::Text text;
    std::string id;
};

class WelcomeScreen {
private:
    sf::Font font;
    std::vector<WelcomeButton> buttons;
    sf::Text title;
    sf::Text status;

public:
    WelcomeScreen();
    void init();
    void updateStatus(bool configured, const std::string& provider);
    void draw(sf::RenderWindow& window);
    std::string handleClick(sf::Vector2f mousePos);
};