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
};

class BrushManager {
private:
    std::map<std::string, BrushPreset> presets;
    std::string activePresetName;

    sf::Vector2f m_prevPoint;
    sf::Vector2f m_prevMidPoint;
    bool m_hasStartedStroke;

    void appendRibbonSegment(sf::RenderTexture* targetTex, sf::Vector2f p1, sf::Vector2f p2, float radius, sf::Color color, float opacity);
    void appendCap(sf::RenderTexture* targetTex, sf::Vector2f center, float radius, sf::Color color, float opacity);

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
    void endStroke(sf::RenderTexture* targetTex, sf::Color color);
    void drawPreviewCursor(sf::RenderWindow& window, sf::Vector2f mousePos, sf::Color color, float scale);
};