#pragma once
#include <SFML/Graphics.hpp>
#include <string>

class NewProjectModal {
private:
    sf::RectangleShape overlay;
    sf::RectangleShape modalBg;
    sf::Font font;
    sf::Text title;

    sf::RectangleShape closeBtn;
    sf::Text closeText;

    sf::RectangleShape normalBtn;
    sf::Text normalText;
    sf::Text normalDesc;

    sf::RectangleShape pixelBtn;
    sf::Text pixelText;
    sf::Text pixelDesc;

    bool isOpen;

public:
    NewProjectModal();
    void init();
    void open();
    void close();
    bool getIsOpen() const;

    void updateHover(sf::Vector2f mousePos);
    std::string handleEvent(const sf::Event& event, sf::RenderWindow& window);
    void draw(sf::RenderWindow& window);
};