#pragma once
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
    sf::Texture brushTexture;
};

class BrushManager {
private:
    std::map<std::string, BrushPreset> presets;
    std::string activePresetName;
    sf::Vector2f lastStabilizedPos;
    sf::Vector2f lastDrawnPos;
    float distanceAccumulator;

    sf::Texture generateBrushTexture(float hardness, int size);
    void applyStamp(sf::RenderTexture* targetTex, sf::Vector2f pos, float currentSize, float currentOpacity, sf::Color color);

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

    void resetStroke(sf::Vector2f startPos);
    void paintStroke(sf::RenderTexture* targetTex, sf::Vector2f targetPos, sf::Color color, float pressure = 1.0f);
    void drawPreviewCursor(sf::RenderWindow& window, sf::Vector2f mousePos, sf::Color color, float scale);
};