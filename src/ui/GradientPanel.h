#pragma once
#include <SFML/Graphics.hpp>
#include "../core/GradientSystem.h"

class GradientPanel {
public:
    GradientPanel();
    void init(GradientConfig* config);
    void update(float dt);
    void draw(sf::RenderWindow& window);
    bool handleEvent(const sf::Event& event, sf::Vector2f mousePos);

    bool wantsColorPanelOpen() const { return m_requestColorPanelOpen; }
    void clearColorPanelRequest() { m_requestColorPanelOpen = false; }
    void setSelectedColor(sf::Color color);

private:
    GradientConfig* m_config;
    sf::RectangleShape m_background;
    sf::Font m_font;

    sf::FloatRect m_gradientBar;
    int m_draggedStopIndex;
    int m_selectedStopIndex;
    bool m_requestColorPanelOpen = false;

    void drawButton(sf::RenderWindow& window, sf::FloatRect bounds, const std::string& text, sf::Color bgColor);
    void drawToggle(sf::RenderWindow& window, sf::FloatRect bounds, const std::string& text, bool state);
    void sortStops();
    void updateBlendMode();
};