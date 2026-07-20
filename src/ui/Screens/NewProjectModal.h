#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

class NewProjectModal {
private:
    sf::RectangleShape overlay;
    sf::RectangleShape modalBg;
    sf::Font font;
    sf::Text title;
    sf::RectangleShape closeBtn;
    sf::Text closeText;

    sf::RectangleShape normalToggleBtn;
    sf::Text normalToggleText;
    sf::RectangleShape pixelToggleBtn;
    sf::Text pixelToggleText;

    sf::Text widthLabel;
    sf::RectangleShape widthBg;
    sf::Text widthText;

    sf::Text heightLabel;
    sf::RectangleShape heightBg;
    sf::Text heightText;

    sf::RectangleShape createBtn;
    sf::Text createText;

    std::vector<sf::RectangleShape> presetBtns;
    std::vector<sf::Text> presetTexts;

    bool isOpen;
    bool isPixelMode;
    int selectedPresetIndex;

    int customWidth;
    int customHeight;
    bool typingWidth;
    bool typingHeight;

    std::string projectName;

    void updateSelectionVisuals();
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