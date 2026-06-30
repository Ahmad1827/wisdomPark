#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include "Canvas.h"

class ExportManager {
public:
    static sf::Image flattenFrame(Canvas& canvas, int frameIndex);
    static sf::IntRect calculateAutoCrop(const sf::Image& img);
    static sf::Image applyCropAndBackground(const sf::Image& img, sf::IntRect cropRect, bool transparentBg);

    static bool exportSingleImage(Canvas& canvas, int currentFrame, const std::string& filepath, bool transparentBg, bool autoCrop);
    static bool exportPNGSequence(Canvas& canvas, const std::string& directoryPath, bool transparentBg, bool autoCrop);
    static bool exportSpriteSheet(Canvas& canvas, const std::string& filepath, int columns, bool transparentBg, bool autoCrop);
};