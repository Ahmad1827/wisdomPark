#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

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
    // More than one entry when the assistant was asked for several options to pick from
    std::vector<sf::Image> options;
    int optionIndex = 0;
    sf::FloatRect prevOptionBounds;
    sf::FloatRect nextOptionBounds;

    void showOption(int index);

    bool isOpen;

public:
    AIReviewModal();
    void init();
    void open(const sf::Image& originalImg, const sf::Image& resultImg);
    void open(const sf::Image& originalImg, const std::vector<sf::Image>& resultOptions);
    int getOptionIndex() const { return optionIndex; }
    int getOptionCount() const { return static_cast<int>(options.size()); }
    void close();
    bool getIsOpen() const;
    const sf::Image& getResultImage() const;
    const sf::Image& getOriginalImage() const;
    void draw(sf::RenderWindow& window);
    std::string handleEvent(const sf::Event& event, sf::Vector2f mousePos);
};