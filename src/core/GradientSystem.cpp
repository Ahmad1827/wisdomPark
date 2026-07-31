#include "GradientSystem.h"
#include <cmath>
#include <algorithm>

bool GradientStop::operator<(const GradientStop& other) const {
    return position < other.position;
}

sf::Color GradientSystem::hsvToRgb(float h, float s, float v, sf::Uint8 a) {
    if (s <= 0.0f) {
        sf::Uint8 val = static_cast<sf::Uint8>(v * 255.0f);
        return sf::Color(val, val, val, a);
    }
    float hh = h;
    if (hh >= 360.0f) hh = 0.0f;
    hh /= 60.0f;
    long i = static_cast<long>(hh);
    float ff = hh - i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - (s * ff));
    float t = v * (1.0f - (s * (1.0f - ff)));

    sf::Uint8 outV = static_cast<sf::Uint8>(v * 255.0f);
    sf::Uint8 outP = static_cast<sf::Uint8>(p * 255.0f);
    sf::Uint8 outQ = static_cast<sf::Uint8>(q * 255.0f);
    sf::Uint8 outT = static_cast<sf::Uint8>(t * 255.0f);

    switch (i) {
    case 0: return sf::Color(outV, outT, outP, a);
    case 1: return sf::Color(outQ, outV, outP, a);
    case 2: return sf::Color(outP, outV, outT, a);
    case 3: return sf::Color(outP, outQ, outV, a);
    case 4: return sf::Color(outT, outP, outV, a);
    case 5: default: return sf::Color(outV, outP, outQ, a);
    }
}

void GradientSystem::rgbToHsv(const sf::Color& c, float& h, float& s, float& v) {
    float r = c.r / 255.0f;
    float g = c.g / 255.0f;
    float b = c.b / 255.0f;
    float minRGB = std::min({ r, g, b });
    float maxRGB = std::max({ r, g, b });
    float delta = maxRGB - minRGB;
    v = maxRGB;
    if (delta < 0.00001f) {
        s = 0;
        h = 0;
        return;
    }
    if (maxRGB > 0.0f) {
        s = delta / maxRGB;
    }
    else {
        s = 0.0f;
        h = NAN;
        return;
    }
    if (r >= maxRGB) h = (g - b) / delta;
    else if (g >= maxRGB) h = 2.0f + (b - r) / delta;
    else h = 4.0f + (r - g) / delta;
    h *= 60.0f;
    if (h < 0.0f) h += 360.0f;
}

sf::Color GradientSystem::interpolate(const GradientConfig& config, float t) {
    if (config.stops.empty()) return sf::Color::Transparent;
    if (config.stops.size() == 1) return config.stops[0].color;

    if (config.reverse) t = 1.0f - t;
    if (config.repeat) {
        t = std::fmod(t, 1.0f);
        if (t < 0.0f) t += 1.0f;
    }
    else {
        t = std::clamp(t, 0.0f, 1.0f);
    }

    auto it = std::lower_bound(config.stops.begin(), config.stops.end(), GradientStop{ t, sf::Color() });
    if (it == config.stops.begin()) return it->color;
    if (it == config.stops.end()) return std::prev(it)->color;

    auto prev = std::prev(it);
    float factor = (t - prev->position) / (it->position - prev->position);

    if (config.interpolation == GradientInterpolation::Constant) {
        return factor < 0.5f ? prev->color : it->color;
    }

    if (config.interpolation == GradientInterpolation::RGB) {
        sf::Uint8 r = static_cast<sf::Uint8>(prev->color.r + factor * (it->color.r - prev->color.r));
        sf::Uint8 g = static_cast<sf::Uint8>(prev->color.g + factor * (it->color.g - prev->color.g));
        sf::Uint8 b = static_cast<sf::Uint8>(prev->color.b + factor * (it->color.b - prev->color.b));
        sf::Uint8 a = static_cast<sf::Uint8>(prev->color.a + factor * (it->color.a - prev->color.a));
        return sf::Color(r, g, b, a);
    }

    float h1, s1, v1, h2, s2, v2;
    rgbToHsv(prev->color, h1, s1, v1);
    rgbToHsv(it->color, h2, s2, v2);

    if (std::abs(h2 - h1) > 180.0f) {
        if (h2 > h1) h1 += 360.0f;
        else h2 += 360.0f;
    }
    float h = h1 + factor * (h2 - h1);
    if (h >= 360.0f) h -= 360.0f;
    float s = s1 + factor * (s2 - s1);
    float v = v1 + factor * (v2 - v1);
    sf::Uint8 a = static_cast<sf::Uint8>(prev->color.a + factor * (it->color.a - prev->color.a));

    return hsvToRgb(h, s, v, a);
}

float GradientSystem::calculateT(GradientType type, sf::Vector2f start, sf::Vector2f end, sf::Vector2f current) {
    sf::Vector2f diff = end - start;
    float lenSq = diff.x * diff.x + diff.y * diff.y;
    if (lenSq < 0.0001f) return 0.0f;

    sf::Vector2f curDiff = current - start;

    switch (type) {
    case GradientType::Linear: {
        return (curDiff.x * diff.x + curDiff.y * diff.y) / lenSq;
    }
    case GradientType::Radial: {
        float dist = std::sqrt(curDiff.x * curDiff.x + curDiff.y * curDiff.y);
        float maxDist = std::sqrt(lenSq);
        return dist / maxDist;
    }
    case GradientType::Diamond: {
        float dist = std::abs(curDiff.x) + std::abs(curDiff.y);
        float maxDist = std::abs(diff.x) + std::abs(diff.y);
        return maxDist > 0.0f ? dist / maxDist : 0.0f;
    }
    case GradientType::Angle: {
        float angle1 = std::atan2(diff.y, diff.x);
        float angle2 = std::atan2(curDiff.y, curDiff.x);
        float angleDiff = angle2 - angle1;
        if (angleDiff < 0.0f) angleDiff += 2.0f * 3.14159265f;
        return angleDiff / (2.0f * 3.14159265f);
    }
    case GradientType::Reflected: {
        float t = (curDiff.x * diff.x + curDiff.y * diff.y) / lenSq;
        return std::abs(t);
    }
    }
    return 0.0f;
}

float GradientSystem::applyDither(float t, int x, int y, GradientDither dither) {
    if (dither == GradientDither::None) return t;

    float threshold = 0.0f;
    int size = 1;

    if (dither == GradientDither::Bayer2x2) {
        static const int bayer[2][2] = { {0, 2}, {3, 1} };
        threshold = (bayer[y % 2][x % 2] + 0.5f) / 4.0f;
        size = 4;
    }
    else if (dither == GradientDither::Bayer4x4) {
        static const int bayer[4][4] = {
            {0, 8, 2, 10},
            {12, 4, 14, 6},
            {3, 11, 1, 9},
            {15, 7, 13, 5}
        };
        threshold = (bayer[y % 4][x % 4] + 0.5f) / 16.0f;
        size = 16;
    }
    else if (dither == GradientDither::Bayer8x8) {
        static const int bayer[8][8] = {
            { 0, 32,  8, 40,  2, 34, 10, 42},
            {48, 16, 56, 24, 50, 18, 58, 26},
            {12, 44,  4, 36, 14, 46,  6, 38},
            {60, 28, 52, 20, 62, 30, 54, 22},
            { 3, 35, 11, 43,  1, 33,  9, 41},
            {51, 19, 59, 27, 49, 17, 57, 25},
            {15, 47,  7, 39, 13, 45,  5, 37},
            {63, 31, 55, 23, 61, 29, 53, 21}
        };
        threshold = (bayer[y % 8][x % 8] + 0.5f) / 64.0f;
        size = 64;
    }

    return t + (threshold - 0.5f) * (1.0f / static_cast<float>(size));
}

sf::Image GradientSystem::generate(const GradientConfig& config, sf::Vector2f start, sf::Vector2f end, sf::Vector2u size, bool isPixelMode, const sf::Image* selectionMask) {
    sf::Image img;
    img.create(size.x, size.y, sf::Color::Transparent);

    for (unsigned int y = 0; y < size.y; ++y) {
        for (unsigned int x = 0; x < size.x; ++x) {
            if (selectionMask && selectionMask->getPixel(x, y).a == 0) continue;

            float t = calculateT(config.type, start, end, sf::Vector2f(static_cast<float>(x), static_cast<float>(y)));
            if (isPixelMode && config.dither != GradientDither::None) {
                t = applyDither(t, x, y, config.dither);
            }

            sf::Color c = interpolate(config, t);
            if (c.a > 0) {
                c.a = static_cast<sf::Uint8>((c.a / 255.0f) * (config.opacity / 100.0f) * 255.0f);
                img.setPixel(x, y, c);
            }
        }
    }
    return img;
}