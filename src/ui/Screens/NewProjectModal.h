#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

class NewProjectModal {
private:
    sf::RectangleShape overlay;
    sf::FloatRect modalBounds;
    sf::Font font;

    sf::FloatRect closeBtnBounds;
    sf::FloatRect normalToggleBounds;
    sf::FloatRect pixelToggleBounds;

    sf::FloatRect widthInputBounds;
    sf::FloatRect heightInputBounds;
    sf::FloatRect nameInputBounds;
    sf::FloatRect createBtnBounds;
    sf::FloatRect previewFrameBounds;

    std::vector<sf::FloatRect> presetBounds;

    bool isOpen;
    bool isPixelMode;
    int selectedPresetIndex;

    int customWidth;
    int customHeight;
    bool typingWidth;
    bool typingHeight;
    bool typingName;

    std::string projectName;

    void buildPresets();

public:
    NewProjectModal();
    void init();
    void open();
    void close();
    bool getIsOpen() const;

    int getWidth() const { return customWidth; }
    int getHeight() const { return customHeight; }
    bool getIsPixelMode() const { return isPixelMode; }
    std::string getProjectName() const { return projectName; }

    void updateHover(sf::Vector2f mousePos);
    std::string handleEvent(const sf::Event& event, sf::RenderWindow& window);
    void draw(sf::RenderWindow& window);
};