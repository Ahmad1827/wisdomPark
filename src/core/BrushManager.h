#ifndef BRUSHMANAGER_H
#define BRUSHMANAGER_H

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <map>

enum class BrushType {
    Pencil,
    Ink,
    Paint,
    Marker,
    PixelBrush,
    Watercolor
};

struct BrushPreset {
    std::string name;
    BrushType type;
    float size;
    float opacity;
    float hardness;
    float spacing;
    float stabilization;
    float smoothing;
    float scatter;
    float rotation;
    bool pressureSensitivity;
    sf::Texture texture;
};

class BrushManager {
private:
    std::map<std::string, BrushPreset> presets;
    BrushPreset activePreset;
    sf::Vector2f lastStabilizedPosition;

public:
    BrushManager();
    ~BrushManager() = default;

    void initDefaultPresets();
    void addPreset(const BrushPreset& preset);
    void selectPreset(const std::string& name);
    BrushPreset& getActivePreset();
    const BrushPreset& getActivePreset() const;

    void setBrushSize(float size);
    void setBrushOpacity(float opacity);
    void setBrushHardness(float hardness);
    void setBrushSpacing(float spacing);
    void setBrushRotation(float rotation);
    void setBrushScatter(float scatter);
    void setStabilization(float value);
    void setSmoothing(float value);

    sf::Vector2f applyStabilization(sf::Vector2f currentTarget, float strength);
    void paintStroke(sf::Image& layerImage, sf::Vector2f start, sf::Vector2f end, sf::Color color, float pressure = 1.0f);
    void updatePreviewCursor(sf::RenderWindow& window, sf::Vector2f mousePos, sf::Color color);
};

#endif