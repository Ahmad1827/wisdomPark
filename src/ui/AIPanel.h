#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include "../ai/AIProvider.h"

class AIPanel {
private:
    sf::Font font;
    sf::Vector2f position;
    sf::Vector2f size;

    bool isDraggingPanel = false;
    sf::Vector2f dragOffset;

    std::string currentPrompt;
    std::string currentNegativePrompt;

    sf::FloatRect promptBoxBounds;
    sf::FloatRect negativePromptBoxBounds;
    sf::FloatRect generateBtnBounds;
    sf::FloatRect backBtnBounds;

    std::vector<sf::FloatRect> opButtonBounds;
    std::vector<std::string> opNames;
    std::vector<AIOperation> opValues;
    AIOperation currentOp;

    bool isVisible;
    bool isTypingPrompt;
    bool isTypingNegative;

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