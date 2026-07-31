#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

enum class GradientType {
    Linear,
    Radial,
    Diamond,
    Angle,
    Reflected
};

enum class GradientInterpolation {
    RGB,
    HSV,
    Constant
};

enum class GradientDither {
    None,
    Bayer2x2,
    Bayer4x4,
    Bayer8x8
};

struct GradientStop {
    float position;
    sf::Color color;
    bool operator<(const GradientStop& other) const;
};

struct GradientConfig {
    GradientType type = GradientType::Linear;
    GradientInterpolation interpolation = GradientInterpolation::RGB;
    GradientDither dither = GradientDither::None;
    sf::BlendMode blendMode = sf::BlendAlpha;
    int blendModeIndex = 1;
    float opacity = 100.0f;
    bool reverse = false;
    bool repeat = false;
    bool livePreview = true;
    bool snapToGrid = false;
    // Set default gradient to fade to Transparent so it doesn't overwrite existing canvas art!
    std::vector<GradientStop> stops = { {0.0f, sf::Color::Black}, {1.0f, sf::Color(0, 0, 0, 0)} };
};

class GradientSystem {
public:
    static sf::Color hsvToRgb(float h, float s, float v, sf::Uint8 a);
    static void rgbToHsv(const sf::Color& c, float& h, float& s, float& v);
    static sf::Color interpolate(const GradientConfig& config, float t, bool applyModifiers = true);
    static float calculateT(GradientType type, sf::Vector2f start, sf::Vector2f end, sf::Vector2f current);
    static float applyDither(float t, int x, int y, GradientDither dither);
    static sf::Image generate(const GradientConfig& config, sf::Vector2f start, sf::Vector2f end, sf::Vector2u size, bool isPixelMode, const sf::Image* selectionMask = nullptr);
};