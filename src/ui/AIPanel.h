#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

struct LospecPalette {
    std::string name;
    std::vector<sf::Color> colors;
};

class AIPanel {
private:
    sf::Font font;
    sf::Vector2f position;
    sf::Vector2f size;

    bool isVisible;
    bool isDraggingPanel;
    sf::Vector2f dragOffset;

    sf::FloatRect headerGripBounds;
    sf::FloatRect dropdownBtnBounds;
    sf::FloatRect suggestBtnBounds;
    sf::FloatRect closeBtnBounds;

    std::vector<LospecPalette> palettes;
    int selectedPaletteIdx;
    bool isDropdownOpen;
    float dropdownScroll;
    float dropdownMaxScroll;
    std::vector<std::pair<sf::FloatRect, int>> dropdownItemBounds;
    sf::FloatRect dropdownListArea;

    std::string pendingAction;

    void loadPalettes();
    sf::Color hexToColor(const std::string& hex) const;
    float getColorDistance(sf::Color c1, sf::Color c2) const;
    float getLuminance(sf::Color c) const;

public:
    AIPanel();
    void init();
    void update(float dt);
    void draw(sf::RenderWindow& window);

    bool handleEvent(const sf::Event& event, sf::Vector2f mousePos);
    std::string handleClick(sf::Vector2f mousePos);

    void toggle();
    bool getIsVisible() const;

    std::vector<sf::Color> generateAdvice(sf::Color base, int variation);
};