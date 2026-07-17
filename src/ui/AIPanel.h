#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include "../ai/AIProvider.h"

class AIPanel {
private:
    sf::RectangleShape background;
    sf::RectangleShape header;
    sf::Text titleText;
    sf::Font font;

    sf::RectangleShape promptBox;
    sf::Text promptText;
    std::string currentPrompt;

    sf::RectangleShape negativePromptBox;
    sf::Text negativePromptText;
    std::string currentNegativePrompt;

    sf::RectangleShape generateBtn;
    sf::Text generateBtnText;

    sf::RectangleShape backBtn;
    sf::Text backBtnText;

    std::vector<sf::RectangleShape> opButtons;
    std::vector<sf::Text> opTexts;
    std::vector<AIOperation> opValues;
    AIOperation currentOp;

    float currentX;
    float targetX;
    float width;
    bool isVisible;
    bool isTypingPrompt;
    bool isTypingNegative;

    void updateTextDisplays();

public:
    AIPanel();
    void init();
    void update(float dt);
    void draw(sf::RenderWindow& window);

    bool handleEvent(const sf::Event& event, sf::Vector2f mousePos);
    std::string handleClick(sf::Vector2f mousePos);

    void toggle();
    bool getIsVisible() const;

    AIRequest buildRequestFromCanvasContext(int w, int h, bool pixelMode, float transparency, bool hasSelection);
};