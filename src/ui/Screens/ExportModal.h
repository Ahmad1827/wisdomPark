#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include "../../core/Canvas.h"
#include "../../core/ExportManager.h"

class ExportModal {
private:
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

    // Filled in by updatePreview() for the details card.
    sf::Vector2u exportSize;
    float previewScale;
    float frameMegabytes;

    bool isOpen;
    bool transparentBg;
    bool autoCrop;

    Canvas* linkedCanvas;
    int activeFrame;

    // Drawing-only state: when the modal opened.
    float openTime;

    void updatePreview();
    void exportFrames();
    void exportSheet();

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
