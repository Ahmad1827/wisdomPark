#pragma once
#include <SFML/Graphics.hpp>
#include "../core/TextSystem.h"

class TextPanel {
public:
    TextPanel();
    void init(TextManager* tm);
    void update(float dt);
    void draw(sf::RenderWindow& window);
    bool handleEvent(const sf::Event& event, sf::Vector2f mousePos);

    bool wantsColorPanelOpen() const { return m_requestColorPanelOpen; }
    void clearColorPanelRequest() { m_requestColorPanelOpen = false; }

private:
    TextManager* m_tm;
    sf::RectangleShape m_background;
    sf::Font m_font;
    bool m_fontDropdownOpen;

    sf::FloatRect m_colorBoxRect;
    bool m_requestColorPanelOpen = false;

    void drawButton(sf::RenderWindow& window, sf::FloatRect bounds, const std::string& text, sf::Color bgColor);
    void drawToggle(sf::RenderWindow& window, sf::FloatRect bounds, const std::string& text, bool state);
};