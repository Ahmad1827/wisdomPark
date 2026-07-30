#include "FontManager.h"
#include <filesystem>
#include <iostream>

FontManager& FontManager::getInstance() {
    static FontManager instance;
    return instance;
}

void FontManager::loadDefaultFonts() {
    importFont("assets/font.otf");
    importFont("C:/Windows/Fonts/arial.ttf");
    importFont("C:/Windows/Fonts/Roboto-Regular.ttf");
}

bool FontManager::importFont(const std::string& path) {
    sf::Font font;
    if (font.loadFromFile(path)) {
        std::string name = std::filesystem::path(path).stem().string();
        m_fonts[name] = font;
        if (std::find(m_fontNames.begin(), m_fontNames.end(), name) == m_fontNames.end()) {
            m_fontNames.push_back(name);
        }
        return true;
    }
    return false;
}

sf::Font* FontManager::getFont(const std::string& name) {
    if (m_fonts.find(name) != m_fonts.end()) {
        return &m_fonts[name];
    }
    if (!m_fonts.empty()) {
        return &m_fonts.begin()->second;
    }
    return nullptr;
}

std::vector<std::string> FontManager::getFontNames() const {
    return m_fontNames;
}