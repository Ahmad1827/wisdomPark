#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include "RightProperties.h"
#include "LayerPanel.h"
#include "ColorPalettePanel.h"
#include "../core/Canvas.h"

class RightPanelManager {
private:
    RightProperties rightProperties;
    LayerPanel layerPanel;
    ColorPalettePanel colorPalettePanel;

public:
    RightPanelManager();
    void init();
    void update(float dt, bool focusMode);
    void updateHover(sf::Vector2f mousePos);
    void draw(sf::RenderWindow& window, Canvas& canvas, int currentFrame);
    std::string handleClick(sf::Vector2f mousePos, Canvas& canvas, int currentFrame);
    float getMinLeftEdge() const;
    void syncPropertiesState(const std::string& theme, bool lighting, bool terrain, bool onion, float onionOpacity, float currentFps);
    bool handlePaletteClick(sf::Vector2f mousePos, sf::Color& outPrimary, sf::Color& outSecondary);
};