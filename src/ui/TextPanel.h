#pragma once
#include <SFML/Graphics.hpp>
#include "../core/TextSystem.h"
#include <string>

class TextPanel {
private:
    TextManager* m_tm;
    sf::Font m_font;
    sf::Vector2f m_position;
    sf::Vector2f m_size;

    bool m_isDraggingPanel = false;
    sf::Vector2f m_dragOffset;
    bool m_fontDropdownOpen;
    sf::FloatRect m_colorBoxRect;
    bool m_requestColorPanelOpen = false;

    void drawButton(sf::RenderWindow& window, sf::FloatRect bounds, const std::string& text, sf::Color bgColor);
    void drawToggle(sf::RenderWindow& window, sf::FloatRect bounds, const std::string& text, bool state);

public:
    TextPanel();
    void init(TextManager* tm);
    void update(float dt);
    void draw(sf::RenderWindow& window);
    bool handleEvent(const sf::Event& event, sf::Vector2f mousePos);

    bool wantsColorPanelOpen() const { return m_requestColorPanelOpen; }
    void clearColorPanelRequest() { m_requestColorPanelOpen = false; }
};