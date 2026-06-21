#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include "../core/KeybindManager.h"

class KeybindSettingsPanel {
private:
    sf::RectangleShape background;
    sf::RectangleShape overlay;
    sf::Font font;
    sf::Text titleText;

    sf::RectangleShape closeBtn;
    sf::Text closeLabel;

    sf::RectangleShape restoreBtn;
    sf::Text restoreLabel;

    KeybindManager* kbm;
    bool isOpen;

    float scrollY;
    std::string listeningId;
    std::string conflictMessage;

public:
    KeybindSettingsPanel();
    void init(KeybindManager* keyManager);

    void toggle();
    void close();
    bool isVisible() const;

    void handleEvent(const sf::Event& event);
    void updateHover(sf::Vector2f mousePos);
    void draw(sf::RenderWindow& window);
};