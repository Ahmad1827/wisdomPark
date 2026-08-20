#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include "../../core/Canvas.h"
#include "../../core/ExportManager.h"

class ExportModal {
private:
    sf::RectangleShape overlay;
    sf::FloatRect modalBounds;
    sf::Font font;

    sf::FloatRect closeBtnBounds;
    sf::FloatRect exportPngBtnBounds;
    sf::FloatRect exportSheetBtnBounds;
    sf::FloatRect transCheckboxBounds;
    sf::FloatRect cropCheckboxBounds;
    sf::FloatRect previewAreaBounds;

    sf::Sprite previewSprite;
    sf::Texture previewTex;
    std::string infoString;

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