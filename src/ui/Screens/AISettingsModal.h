#pragma once
#include <SFML/Graphics.hpp>
#include "../../core/SettingsManager.h"
#include <string>
#include <vector>

struct ModalButton {
    sf::RectangleShape rect;
    sf::Text text;
    std::string id;
    bool isHovered = false;
};

class AISettingsModal {
private:
    sf::RectangleShape overlay;
    sf::RectangleShape modalBg;
    sf::Font font;
    sf::Text title;
    sf::Text statusText;

    std::vector<ModalButton> providerButtons;
    ModalButton saveButton;
    ModalButton closeButton;

    sf::RectangleShape inputBox;
    sf::Text inputText;
    sf::Text inputLabel;

    bool isOpen;
    std::string inputBuffer;
    std::string selectedProvider;

    void updateProviderButtons();

public:
    AISettingsModal();
    void init();
    void open(const AppSettings& currentSettings);
    void close();
    bool getIsOpen() const;

    void updateHover(sf::Vector2f mousePos);
    void handleTextEntered(sf::Uint32 unicode);
    void handleKeyPress(sf::Keyboard::Key key, AppSettings& settings);
    bool handleClick(sf::Vector2f mousePos, AppSettings& settings);

    void draw(sf::RenderWindow& window);
};