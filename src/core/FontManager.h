#pragma once
#include <SFML/Graphics.hpp>
#include <map>
#include <string>
#include <vector>

class FontManager {
public:
    static FontManager& getInstance();
    void loadDefaultFonts();
    bool importFont(const std::string& path);
    sf::Font* getFont(const std::string& name);
    std::vector<std::string> getFontNames() const;

private:
    FontManager() = default;
    std::map<std::string, sf::Font> m_fonts;
    std::vector<std::string> m_fontNames;
};