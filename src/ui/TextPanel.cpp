#include "TextPanel.h"
#include "../core/FontManager.h"
#include "../core/NativeDialogs.h"
#include "../UI/UITheme.h"
#include <algorithm>
#include <cmath>

TextPanel::TextPanel() : m_tm(nullptr), m_position(64.f, 78.f), m_size(280.f, 510.f), m_fontDropdownOpen(false) {}

void TextPanel::init(TextManager* tm) {
    m_tm = tm;
    m_font.loadFromFile("assets/font.otf");
    m_position = sf::Vector2f(64.f, 78.f);
}

void TextPanel::update(float dt) {}

void TextPanel::drawButton(sf::RenderWindow& window, sf::FloatRect bounds, const std::string& text, sf::Color bgColor) {
    bool hovered = bounds.contains(window.mapPixelToCoords(sf::Mouse::getPosition(window)));
    WisdomUI::Theme::DrawSunsetButton(window, bounds, text, m_font, 12, false, hovered, false, 1.0f);
}

void TextPanel::drawToggle(sf::RenderWindow& window, sf::FloatRect bounds, const std::string& text, bool state) {
    bool hovered = bounds.contains(window.mapPixelToCoords(sf::Mouse::getPosition(window)));
    WisdomUI::Theme::DrawSunsetButton(window, bounds, text, m_font, 12, state, hovered, state, 1.0f);
}

void TextPanel::draw(sf::RenderWindow& window) {
    sf::FloatRect panelBounds(m_position.x, m_position.y, m_size.x, m_size.y);
    WisdomUI::Theme::DrawSunsetPanel(window, panelBounds, 1.0f);

    sf::FloatRect headerGrip(m_position.x + 8.f, m_position.y + 6.f, m_size.x - 16.f, 28.f);
    sf::RectangleShape gripBg(sf::Vector2f(headerGrip.width, headerGrip.height));
    gripBg.setPosition(headerGrip.left, headerGrip.top);
    gripBg.setFillColor(WisdomUI::Theme::SunsetDeepDark);
    gripBg.setOutlineThickness(1.f);
    gripBg.setOutlineColor(WisdomUI::Theme::SunsetPlum);
    window.draw(gripBg);

    WisdomUI::Theme::DrawCrispText(window, m_font, ":: TEXT PROPERTIES ::", 13, headerGrip.left + headerGrip.width / 2.0f, headerGrip.top + headerGrip.height / 2.0f, WisdomUI::Theme::SunsetAmber, sf::Color(14, 6, 20), true, true);

    TextObject* activeText = m_tm ? m_tm->getEditingText() : nullptr;
    float y = m_position.y + 42.f;

    drawButton(window, sf::FloatRect(m_position.x + 12.f, y, 122.f, 28.f), "Import Font", WisdomUI::Theme::SunsetCoral);
    drawButton(window, sf::FloatRect(m_position.x + 144.f, y, 122.f, 28.f), activeText ? activeText->fontName : "Font...", WisdomUI::Theme::SunsetSkyMid);
    y += 36.f;

    if (m_fontDropdownOpen) {
        auto names = FontManager::getInstance().getFontNames();
        for (const auto& name : names) {
            drawButton(window, sf::FloatRect(m_position.x + 12.f, y, 254.f, 24.f), name, WisdomUI::Theme::SunsetSkyTop);
            y += 28.f;
        }
        y += 6.f;
    }

    if (activeText) {
        drawButton(window, sf::FloatRect(m_position.x + 12.f, y, 62.f, 28.f), "Size-", WisdomUI::Theme::SunsetCoralDark);
        drawButton(window, sf::FloatRect(m_position.x + 80.f, y, 118.f, 28.f), std::to_string(activeText->size) + " px", WisdomUI::Theme::SunsetSkyTop);
        drawButton(window, sf::FloatRect(m_position.x + 204.f, y, 62.f, 28.f), "Size+", WisdomUI::Theme::SunsetCoralDark);
        y += 36.f;

        drawToggle(window, sf::FloatRect(m_position.x + 12.f, y, 58.f, 28.f), "B", activeText->bold);
        drawToggle(window, sf::FloatRect(m_position.x + 76.f, y, 58.f, 28.f), "I", activeText->italic);
        drawToggle(window, sf::FloatRect(m_position.x + 140.f, y, 58.f, 28.f), "U", activeText->underline);
        drawToggle(window, sf::FloatRect(m_position.x + 204.f, y, 62.f, 28.f), "S", activeText->strikethrough);
        y += 36.f;

        drawToggle(window, sf::FloatRect(m_position.x + 12.f, y, 122.f, 28.f), "Outline", activeText->outline);
        drawToggle(window, sf::FloatRect(m_position.x + 144.f, y, 122.f, 28.f), "Shadow", activeText->shadow);
        y += 36.f;

        drawToggle(window, sf::FloatRect(m_position.x + 12.f, y, 254.f, 28.f), "Background Box", activeText->box);
        y += 36.f;

        WisdomUI::Theme::DrawCrispText(window, m_font, "Text Color:", 13, m_position.x + 14.f, y + 5.f, WisdomUI::Theme::TextSecondary);

        m_colorBoxRect = sf::FloatRect(m_position.x + 120.f, y, 146.f, 26.f);
        sf::RectangleShape colorBox(sf::Vector2f(m_colorBoxRect.width, m_colorBoxRect.height));
        colorBox.setPosition(m_colorBoxRect.left, m_colorBoxRect.top);
        colorBox.setFillColor(activeText->color);
        colorBox.setOutlineThickness(1.5f);
        colorBox.setOutlineColor(WisdomUI::Theme::SunsetAmber);
        window.draw(colorBox);
    }
}

bool TextPanel::handleEvent(const sf::Event& event, sf::Vector2f mousePos) {
    sf::FloatRect headerGrip(m_position.x, m_position.y, m_size.x, 34.f);

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        if (headerGrip.contains(mousePos)) {
            m_isDraggingPanel = true;
            m_dragOffset = mousePos - m_position;
            return true;
        }

        float effectiveH = m_size.y;
        if (m_fontDropdownOpen) {
            effectiveH += FontManager::getInstance().getFontNames().size() * 28.f + 20.f;
        }

        if (!sf::FloatRect(m_position.x, m_position.y, m_size.x, effectiveH).contains(mousePos)) {
            if (m_fontDropdownOpen) m_fontDropdownOpen = false;
            return false;
        }

        float bx = m_position.x;
        float y = m_position.y + 42.f;

        if (sf::FloatRect(bx + 12.f, y, 122.f, 28.f).contains(mousePos)) {
            std::string path = NativeDialogs::openFileDialog("Font Files\0*.ttf;*.otf\0");
            if (!path.empty()) FontManager::getInstance().importFont(path);
            return true;
        }
        if (sf::FloatRect(bx + 144.f, y, 122.f, 28.f).contains(mousePos)) {
            m_fontDropdownOpen = !m_fontDropdownOpen;
            return true;
        }
        y += 36.f;

        TextObject* activeText = m_tm ? m_tm->getEditingText() : nullptr;

        if (m_fontDropdownOpen) {
            auto names = FontManager::getInstance().getFontNames();
            for (const auto& name : names) {
                if (sf::FloatRect(bx + 12.f, y, 254.f, 24.f).contains(mousePos)) {
                    if (activeText) activeText->fontName = name;
                    m_fontDropdownOpen = false;
                    return true;
                }
                y += 28.f;
            }
            y += 6.f;
        }

        if (activeText) {
            if (sf::FloatRect(bx + 12.f, y, 62.f, 28.f).contains(mousePos)) { activeText->size = std::max(8, activeText->size - 2); return true; }
            if (sf::FloatRect(bx + 204.f, y, 62.f, 28.f).contains(mousePos)) { activeText->size = std::min(512, activeText->size + 2); return true; }
            y += 36.f;

            if (sf::FloatRect(bx + 12.f, y, 58.f, 28.f).contains(mousePos)) { activeText->bold = !activeText->bold; return true; }
            if (sf::FloatRect(bx + 76.f, y, 58.f, 28.f).contains(mousePos)) { activeText->italic = !activeText->italic; return true; }
            if (sf::FloatRect(bx + 140.f, y, 58.f, 28.f).contains(mousePos)) { activeText->underline = !activeText->underline; return true; }
            if (sf::FloatRect(bx + 204.f, y, 62.f, 28.f).contains(mousePos)) { activeText->strikethrough = !activeText->strikethrough; return true; }
            y += 36.f;

            if (sf::FloatRect(bx + 12.f, y, 122.f, 28.f).contains(mousePos)) { activeText->outline = !activeText->outline; return true; }
            if (sf::FloatRect(bx + 144.f, y, 122.f, 28.f).contains(mousePos)) { activeText->shadow = !activeText->shadow; return true; }
            y += 36.f;

            if (sf::FloatRect(bx + 12.f, y, 254.f, 28.f).contains(mousePos)) { activeText->box = !activeText->box; return true; }
            y += 36.f;

            if (m_colorBoxRect.contains(mousePos)) {
                m_requestColorPanelOpen = true;
                return true;
            }
        }
        return true;
    }
    else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        m_isDraggingPanel = false;
    }
    else if (event.type == sf::Event::MouseMoved && m_isDraggingPanel) {
        m_position = mousePos - m_dragOffset;
        m_position.x = std::clamp(m_position.x, 56.f, 1920.f - m_size.x);
        m_position.y = std::clamp(m_position.y, 40.f, 1080.f - m_size.y);
        return true;
    }
    return false;
}