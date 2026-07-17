#pragma once
#include <SFML/Graphics.hpp>
#include <string>

class AIReviewModal {
private:
    sf::RectangleShape overlay;
    sf::RectangleShape modalBg;
    sf::Text titleText;
    sf::Font font;

    sf::RectangleShape originalView;
    sf::RectangleShape resultView;
    sf::Texture originalTexture;
    sf::Texture resultTexture;

    sf::RectangleShape acceptNewLayerBtn;
    sf::Text acceptNewLayerText;

    sf::RectangleShape replaceLayerBtn;
    sf::Text replaceLayerText;

    sf::RectangleShape newProjectBtn;
    sf::Text newProjectText;

    sf::RectangleShape rejectBtn;
    sf::Text rejectText;

    sf::Image originalSavedImage;
    sf::Image resultSavedImage;

    bool isOpen;

public:
    AIReviewModal();
    void init();
    void open(const sf::Image& originalImg, const sf::Image& resultImg);
    void close();
    bool getIsOpen() const;
    const sf::Image& getResultImage() const;
    void draw(sf::RenderWindow& window);
    std::string handleEvent(const sf::Event& event, sf::Vector2f mousePos);
};