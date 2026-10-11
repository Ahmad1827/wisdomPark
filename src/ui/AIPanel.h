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
    sf::FloatRect pasteBtnBounds;
    sf::FloatRect randomBtnBounds;
    sf::FloatRect suggestBtnBounds;
    sf::FloatRect sendAllBtnBounds;
    sf::FloatRect aiFinishBtnBounds;
    sf::FloatRect aiContourBtnBounds;
    sf::FloatRect aiAskBtnBounds;
    sf::FloatRect aiColorsBtnBounds;
    sf::FloatRect aiNextFrameBtnBounds;
    sf::FloatRect aiOutputBtnBounds;
    int assistOutputMode = 0;
    void loadAssistPrefs();
    void saveAssistPrefs() const;
    sf::FloatRect aiOptionsBtnBounds;
    int assistColorMode = 0;
    int assistOptionCount = 1;
    sf::FloatRect aiFrameCountBtnBounds;
    int assistFrameCount = 1;
    sf::FloatRect closeBtnBounds;

    std::vector<std::pair<sf::FloatRect, sf::Color>> paletteSwatchBounds;
    sf::Color lastPickedColor;

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
    std::string colorToHex(sf::Color c) const;
    float getColorDistance(sf::Color c1, sf::Color c2) const;
    float getLuminance(sf::Color c) const;
    void drawTooltip(sf::RenderWindow& window, const std::string& text, sf::Vector2f pos);

public:
    AIPanel();
    void init();
    void update(float dt);
    void draw(sf::RenderWindow& window);

    bool handleEvent(const sf::Event& event, sf::Vector2f mousePos);
    std::string handleClick(sf::Vector2f mousePos);

    void toggle();
    bool getIsVisible() const;

    bool importFromClipboard();
    void pickRandomPalette();
    std::string getSelectedPaletteName() const;
    const std::vector<sf::Color>& getSelectedPaletteColors() const;
    sf::Color getLastPickedColor() const;

    // What the assistant may colour with: 0 = anything, 1 = only colours already in the drawing, 2 = only the selected palette
    int getAssistColorMode() const { return assistColorMode; }
    // How many alternatives the assistant makes per request (1 to 3)
    int getAssistOptionCount() const { return assistOptionCount; }
    // How many frames Draw Next Frame makes in one go (1 to 4)
    int getAssistFrameCount() const { return assistFrameCount; }
    // true = the assistant answers with only the pixels it changes (quicker, larger canvases); false = it redraws the whole canvas
    bool getAssistChangesOnly() const { return assistOutputMode == 0; }
    bool containsPoint(sf::Vector2f point) const { return isVisible && sf::FloatRect(position.x, position.y, size.x, size.y).contains(point); }

    std::vector<sf::Color> generateAdvice(sf::Color base, int variation);
};