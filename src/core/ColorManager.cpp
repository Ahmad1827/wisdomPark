#include "ColorManager.h"
#include <fstream>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <filesystem>

ColorManager::ColorManager() : paletteFilePath("projects/custom_palette.txt") {}

void ColorManager::init() {
    if (!std::filesystem::exists("projects")) {
        std::filesystem::create_directory("projects");
    }
    loadPalette();
    if (customSwatches.empty()) {
        customSwatches = {
            sf::Color(0, 0, 0), sf::Color(255, 255, 255), sf::Color(157, 157, 157),
            sf::Color(255, 0, 68), sf::Color(250, 166, 19), sf::Color(255, 219, 0),
            sf::Color(146, 224, 0), sf::Color(26, 186, 0), sf::Color(26, 203, 208),
            sf::Color(0, 114, 213), sf::Color(111, 48, 218), sf::Color(204, 76, 250)
        };
        savePalette();
    }
}

void ColorManager::savePalette() {
    std::ofstream out(paletteFilePath);
    if (out.is_open()) {
        for (const auto& c : customSwatches) {
            out << static_cast<int>(c.r) << " " << static_cast<int>(c.g) << " " << static_cast<int>(c.b) << " " << static_cast<int>(c.a) << "\n";
        }
    }
}

void ColorManager::loadPalette() {
    std::ifstream in(paletteFilePath);
    if (in.is_open()) {
        int r, g, b, a;
        customSwatches.clear();
        while (in >> r >> g >> b >> a) {
            customSwatches.push_back(sf::Color(r, g, b, a));
        }
    }
}

void ColorManager::addRecentColor(sf::Color color) {
    auto it = std::find(recentColors.begin(), recentColors.end(), color);
    if (it != recentColors.end()) {
        recentColors.erase(it);
    }
    recentColors.insert(recentColors.begin(), color);
    if (recentColors.size() > 24) {
        recentColors.pop_back();
    }
}

const std::vector<sf::Color>& ColorManager::getRecentColors() const {
    return recentColors;
}

void ColorManager::addCustomSwatch(sf::Color color) {
    auto it = std::find(customSwatches.begin(), customSwatches.end(), color);
    if (it == customSwatches.end()) {
        customSwatches.push_back(color);
        savePalette();
    }
}

void ColorManager::removeCustomSwatch(size_t index) {
    if (index < customSwatches.size()) {
        customSwatches.erase(customSwatches.begin() + index);
        savePalette();
    }
}

const std::vector<sf::Color>& ColorManager::getCustomSwatches() const {
    return customSwatches;
}

sf::Color ColorManager::hsvToRgb(float h, float s, float v) {
    float c = v * s;
    float x = c * (1.0f - std::abs(std::fmod(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;
    float r = 0, g = 0, b = 0;

    if (h >= 0 && h < 60) { r = c; g = x; b = 0; }
    else if (h >= 60 && h < 120) { r = x; g = c; b = 0; }
    else if (h >= 120 && h < 180) { r = 0; g = c; b = x; }
    else if (h >= 180 && h < 240) { r = 0; g = x; b = c; }
    else if (h >= 240 && h < 300) { r = x; g = 0; b = c; }
    else { r = c; g = 0; b = x; }

    return sf::Color(static_cast<sf::Uint8>((r + m) * 255.0f), static_cast<sf::Uint8>((g + m) * 255.0f), static_cast<sf::Uint8>((b + m) * 255.0f));
}

void ColorManager::rgbToHsv(sf::Color c, float& outH, float& outS, float& outV) {
    float r = c.r / 255.0f;
    float g = c.g / 255.0f;
    float b = c.b / 255.0f;

    float cmax = std::max(r, std::max(g, b));
    float cmin = std::min(r, std::min(g, b));
    float delta = cmax - cmin;

    outH = 0;
    if (delta > 0) {
        if (cmax == r) outH = 60.0f * std::fmod(((g - b) / delta), 6.0f);
        else if (cmax == g) outH = 60.0f * (((b - r) / delta) + 2.0f);
        else outH = 60.0f * (((r - g) / delta) + 4.0f);
    }
    if (outH < 0) outH += 360.0f;

    outS = (cmax == 0) ? 0 : (delta / cmax);
    outV = cmax;
}

std::array<sf::Color, 1> ColorManager::getComplementary(sf::Color c) {
    float h, s, v;
    rgbToHsv(c, h, s, v);
    float compH = std::fmod(h + 180.0f, 360.0f);
    return { hsvToRgb(compH, s, v) };
}

std::array<sf::Color, 2> ColorManager::getAnalogous(sf::Color c) {
    float h, s, v;
    rgbToHsv(c, h, s, v);
    float a1 = std::fmod(h + 30.0f, 360.0f);
    float a2 = std::fmod(h + 330.0f, 360.0f);
    return { hsvToRgb(a1, s, v), hsvToRgb(a2, s, v) };
}

std::array<sf::Color, 2> ColorManager::getTriadic(sf::Color c) {
    float h, s, v;
    rgbToHsv(c, h, s, v);
    float t1 = std::fmod(h + 120.0f, 360.0f);
    float t2 = std::fmod(h + 240.0f, 360.0f);
    return { hsvToRgb(t1, s, v), hsvToRgb(t2, s, v) };
}

std::array<sf::Color, 3> ColorManager::getMonochromatic(sf::Color c) {
    float h, s, v;
    rgbToHsv(c, h, s, v);
    return {
        hsvToRgb(h, std::clamp(s - 0.3f, 0.0f, 1.0f), std::clamp(v + 0.2f, 0.0f, 1.0f)),
        hsvToRgb(h, std::clamp(s + 0.2f, 0.0f, 1.0f), std::clamp(v - 0.3f, 0.0f, 1.0f)),
        hsvToRgb(h, std::clamp(s - 0.5f, 0.0f, 1.0f), std::clamp(v - 0.5f, 0.0f, 1.0f))
    };
}