#include "DitherManager.h"

DitherManager::DitherManager() : currentPattern(DitherPattern::Bayer4x4), currentDensity(0.5f) {}

void DitherManager::setPattern(DitherPattern pattern) { currentPattern = pattern; }
void DitherManager::setDensity(float density) { currentDensity = density; }

bool DitherManager::shouldDrawPixel(int x, int y) const {
    if (currentDensity >= 1.0f) return true;
    if (currentDensity <= 0.0f) return false;
    return currentDensity > getThreshold(x, y);
}

float DitherManager::getThreshold(int x, int y) const {
    if (currentPattern == DitherPattern::Checkerboard) {
        return ((x + y) % 2 == 0) ? 0.25f : 0.75f;
    }

    int bayer4x4[4][4] = {
        { 0, 8, 2, 10 },
        { 12, 4, 14, 6 },
        { 3, 11, 1, 9 },
        { 15, 7, 13, 5 }
    };

    if (currentPattern == DitherPattern::Bayer4x4) {
        return (bayer4x4[y % 4][x % 4] + 0.5f) / 16.0f;
    }

    int bayer2x2[2][2] = {
        { 0, 2 },
        { 3, 1 }
    };

    if (currentPattern == DitherPattern::Bayer2x2) {
        return (bayer2x2[y % 2][x % 2] + 0.5f) / 4.0f;
    }

    return 0.5f;
}