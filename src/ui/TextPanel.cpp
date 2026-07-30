#include "TextPanel.h"
#include "../core/FontManager.h"
#include "../core/NativeDialogs.h"

TextPanel::TextPanel() : m_tm(nullptr), m_fontDropdownOpen(false) {}

void TextPanel::init(TextManager* tm) {
    m_tm = tm;
    m_font.loadFromFile("assets/font.otf");
    m_background.setSize(sf::Vector2f(260.f, 500.f));
    m_background.setFillColor(sf::Color(25, 25, 30, 240));
    m_background.setOutlineThickness(1.f);
    m_background.setOutlineColor(sf::Color(100, 100, 110));
    m_background.setPosition(15.f, 300.f);
}

void TextPanel::update(float dt) {}

void TextPanel::drawButton(sf::RenderWindow& window, sf::FloatRect bounds, const std::string& text, sf::Color bgColor) {
    sf::RectangleShape btn(sf::Vector2f(bounds.width, bounds.height));
    btn.setPosition(bounds.left, bounds.top);
    btn.setFillColor(bgColor);
    btn.setOutlineThickness(1.f);
    btn.setOutlineColor(sf::Color(80, 80, 90));
    window.draw(btn);
    sf::Text t(text, m_font, 12);
    t.setPosition(bounds.left + 10.f, bounds.top + 6.f);
    t.setFillColor(sf::Color::White);
    window.draw(t);
}

void TextPanel::drawToggle(sf::RenderWindow& window, sf::FloatRect bounds, const std::string& text, bool state) {
    drawButton(window, bounds, text, state ? sf::Color(0, 122, 204) : sf::Color(40, 40, 45));
}

void TextPanel::draw(sf::RenderWindow& window) {
    window.draw(m_background);
    sf::Text title("TEXT PROPERTIES", m_font, 14);
    title.setPosition(m_background.getPosition().x + 10.f, m_background.getPosition().y + 10.f);
    title.setFillColor(sf::Color(255, 200, 100));
    window.draw(title);

    TextObject* activeText = m_tm->getEditingText();
    float y = m_background.getPosition().y + 40.f;

    drawButton(window, sf::FloatRect(m_background.getPosition().x + 10.f, y, 115.f, 25.f), "Import Font", sf::Color(50, 150, 50));
    drawButton(window, sf::FloatRect(m_background.getPosition().x + 135.f, y, 115.f, 25.f), activeText ? activeText->fontName : "Font...", sf::Color(50, 50, 60));
    y += 35.f;

    if (m_fontDropdownOpen) {
        auto names = FontManager::getInstance().getFontNames();
        for (const auto& name : names) {
            drawButton(window, sf::FloatRect(m_background.getPosition().x + 10.f, y, 240.f, 20.f), name, sf::Color(30, 30, 35));
            y += 22.f;
        }
        y += 10.f;
    }

    if (activeText) {
        drawButton(window, sf::FloatRect(m_background.getPosition().x + 10.f, y, 60.f, 25.f), "Size-", sf::Color(50, 50, 60));
        drawButton(window, sf::FloatRect(m_background.getPosition().x + 75.f, y, 115.f, 25.f), std::to_string(activeText->size) + " px", sf::Color(40, 40, 45));
        drawButton(window, sf::FloatRect(m_background.getPosition().x + 195.f, y, 55.f, 25.f), "Size+", sf::Color(50, 50, 60));
        y += 35.f;

        drawToggle(window, sf::FloatRect(m_background.getPosition().x + 10.f, y, 55.f, 25.f), "B", activeText->bold);
        drawToggle(window, sf::FloatRect(m_background.getPosition().x + 70.f, y, 55.f, 25.f), "I", activeText->italic);
        drawToggle(window, sf::FloatRect(m_background.getPosition().x + 130.f, y, 55.f, 25.f), "U", activeText->underline);
        drawToggle(window, sf::FloatRect(m_background.getPosition().x + 190.f, y, 60.f, 25.f), "S", activeText->strikethrough);
        y += 35.f;

        drawToggle(window, sf::FloatRect(m_background.getPosition().x + 10.f, y, 115.f, 25.f), "Outline", activeText->outline);
        drawToggle(window, sf::FloatRect(m_background.getPosition().x + 135.f, y, 115.f, 25.f), "Shadow", activeText->shadow);
        y += 35.f;

        drawToggle(window, sf::FloatRect(m_background.getPosition().x + 10.f, y, 240.f, 25.f), "Background Box", activeText->box);
    }
}

void TextPanel::handleEvent(const sf::Event& event, sf::Vector2f mousePos) {
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        if (!m_background.getGlobalBounds().contains(mousePos)) return;

        float bx = m_background.getPosition().x;
        float y = m_background.getPosition().y + 40.f;

        if (sf::FloatRect(bx + 10.f, y, 115.f, 25.f).contains(mousePos)) {
            std::string path = NativeDialogs::openFileDialog("Font Files\0*.ttf;*.otf\0");
            if (!path.empty()) FontManager::getInstance().importFont(path);
            return;
        }
        if (sf::FloatRect(bx + 135.f, y, 115.f, 25.f).contains(mousePos)) m_fontDropdownOpen = !m_fontDropdownOpen;
        y += 35.f;

        TextObject* activeText = m_tm->getEditingText();

        if (m_fontDropdownOpen) {
            auto names = FontManager::getInstance().getFontNames();
            for (const auto& name : names) {
                if (sf::FloatRect(bx + 10.f, y, 240.f, 20.f).contains(mousePos)) {
                    if (activeText) activeText->fontName = name;
                    m_fontDropdownOpen = false;
                    return;
                }
                y += 22.f;
            }
            y += 10.f;
        }

        if (activeText) {
            if (sf::FloatRect(bx + 10.f, y, 60.f, 25.f).contains(mousePos)) activeText->size = std::max(8, activeText->size - 2);
            if (sf::FloatRect(bx + 195.f, y, 55.f, 25.f).contains(mousePos)) activeText->size = std::min(512, activeText->size + 2);
            y += 35.f;

            if (sf::FloatRect(bx + 10.f, y, 55.f, 25.f).contains(mousePos)) activeText->bold = !activeText->bold;
            if (sf::FloatRect(bx + 70.f, y, 55.f, 25.f).contains(mousePos)) activeText->italic = !activeText->italic;
            if (sf::FloatRect(bx + 130.f, y, 55.f, 25.f).contains(mousePos)) activeText->underline = !activeText->underline;
            if (sf::FloatRect(bx + 190.f, y, 60.f, 25.f).contains(mousePos)) activeText->strikethrough = !activeText->strikethrough;
            y += 35.f;

            if (sf::FloatRect(bx + 10.f, y, 115.f, 25.f).contains(mousePos)) activeText->outline = !activeText->outline;
            if (sf::FloatRect(bx + 135.f, y, 115.f, 25.f).contains(mousePos)) activeText->shadow = !activeText->shadow;
            y += 35.f;

            if (sf::FloatRect(bx + 10.f, y, 240.f, 25.f).contains(mousePos)) activeText->box = !activeText->box;
        }
    }
}