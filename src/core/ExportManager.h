#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include "Canvas.h"

class ExportManager {
public:
    static bool exportPNGSequence(Canvas& canvas, const std::string& directoryPath);
    static bool exportSpriteSheet(Canvas& canvas, const std::string& filepath, int columns);
};