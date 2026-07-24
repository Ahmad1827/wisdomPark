#pragma once
#include <vector>

enum class DitherPattern { Bayer2x2, Bayer4x4, Bayer8x8, Checkerboard };

class DitherManager {
public:
    DitherManager();
    void setPattern(DitherPattern pattern);
    void setDensity(float density);
    bool shouldDrawPixel(int x, int y) const;

private:
    DitherPattern currentPattern;
    float currentDensity;
    float getThreshold(int x, int y) const;
};