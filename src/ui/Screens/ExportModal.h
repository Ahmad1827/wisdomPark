#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include "../../core/Canvas.h"
#include "../../core/ExportManager.h"

class ExportModal {
private:
    sf::RectangleShape overlay;
    sf::RectangleShape modalBg;
    sf::Font font;
    sf::Text title;

    sf::RectangleShape closeBtn;
    sf::Text closeText;

    sf::RectangleShape exportBtn;
    sf::Text exportText;

    sf::RectangleShape transCheckbox;
    sf::Text transText;
    sf::RectangleShape cropCheckbox;
    sf::Text cropText;

    sf::RectangleShape previewArea;
    sf::Sprite previewSprite;
    sf::Texture previewTex;

    sf::Text infoText;

    bool isOpen;
    bool transparentBg;
    bool autoCrop;

    Canvas* linkedCanvas;
    int activeFrame;

    void updatePreview();

public:
    ExportModal();
    void init();
    void open(Canvas& canvas, int frameIndex);
    void close();
    bool getIsOpen() const;

    void updateHover(sf::Vector2f mousePos);
    void handleEvent(const sf::Event& event, sf::RenderWindow& window);
    void draw(sf::RenderWindow& window);
};