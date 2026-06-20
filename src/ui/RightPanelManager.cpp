#include "RightPanelManager.h"
#include <algorithm>

RightPanelManager::RightPanelManager() {}

void RightPanelManager::init() {
    rightProperties.init();
    layerPanel.init();
    colorPalettePanel.init();
}

void RightPanelManager::update(float dt, bool focusMode) {
    rightProperties.update(dt, focusMode);
    layerPanel.update(dt, focusMode);
    colorPalettePanel.update(dt, focusMode);
}

void RightPanelManager::updateHover(sf::Vector2f mousePos) {
    rightProperties.updateHover(mousePos);
    layerPanel.updateHover(mousePos);
    colorPalettePanel.updateHover(mousePos);

    if (rightProperties.isHovered() && !rightProperties.isPanelPinned()) {
        if (!layerPanel.isPanelPinned()) layerPanel.forceClose();
        if (!colorPalettePanel.isPanelPinned()) colorPalettePanel.forceClose();
    }
    else if (layerPanel.isHovered() && !layerPanel.isPanelPinned()) {
        if (!rightProperties.isPanelPinned()) rightProperties.forceClose();
        if (!colorPalettePanel.isPanelPinned()) colorPalettePanel.forceClose();
    }
    else if (colorPalettePanel.isHovered() && !colorPalettePanel.isPanelPinned()) {
        if (!rightProperties.isPanelPinned()) rightProperties.forceClose();
        if (!layerPanel.isPanelPinned()) layerPanel.forceClose();
    }
}

void RightPanelManager::draw(sf::RenderWindow& window, Canvas& canvas, int currentFrame) {
    rightProperties.draw(window);
    layerPanel.draw(window, canvas, currentFrame);
    colorPalettePanel.draw(window);
}

std::string RightPanelManager::handleClick(sf::Vector2f mousePos, Canvas& canvas, int currentFrame) {
    std::string action = rightProperties.handleClick(mousePos);
    if (!action.empty()) return action;

    if (layerPanel.handleClick(mousePos, canvas, currentFrame)) return "layer_action";

    return "";
}

float RightPanelManager::getMinLeftEdge() const {
    float minEdge = 1920.f;
    if (rightProperties.getCurrentX() < minEdge) minEdge = rightProperties.getCurrentX();
    if (layerPanel.getCurrentX() < minEdge) minEdge = layerPanel.getCurrentX();
    if (colorPalettePanel.getCurrentX() < minEdge) minEdge = colorPalettePanel.getCurrentX();
    return minEdge;
}

void RightPanelManager::syncPropertiesState(const std::string& theme, bool lighting, bool terrain, bool onion, float onionOpacity) {
    rightProperties.syncState(theme, lighting, terrain, onion, onionOpacity);
}

bool RightPanelManager::handlePaletteClick(sf::Vector2f mousePos, sf::Color& outPrimary, sf::Color& outSecondary) {
    return colorPalettePanel.handleClick(mousePos, outPrimary, outSecondary);
}