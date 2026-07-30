#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

enum class PerspectiveMode { OnePoint, TwoPoint, ThreePoint, Custom, Isometric };

class VanishingPoint {
public:
    sf::Vector2f position;
    bool locked;
    bool visible;
    sf::Color color;
    std::string name;

    VanishingPoint(sf::Vector2f pos, std::string n);
};

class PerspectiveGuide {
public:
    int density;
    float thickness;
    sf::Color guideColor;
    sf::Color activeColor;
    bool brushSnap;
    bool shapeSnap;
    bool transformSnap;
    bool selectionSnap;
    bool visible;
    bool locked;

    PerspectiveGuide();
};

class PerspectiveConfig {
public:
    std::string name;
    PerspectiveMode mode;
    std::vector<VanishingPoint> vps;
    PerspectiveGuide guideSettings;

    PerspectiveConfig(std::string n, PerspectiveMode m);
};

class PerspectiveSnapper {
public:
    static sf::Vector2f snapLine(sf::Vector2f startPos, sf::Vector2f currentPos, const PerspectiveConfig& config, int& outActiveVPIndex);
    static sf::Vector2f snapToPixelGrid(sf::Vector2f pos);
};

class PerspectiveRenderer {
public:
    static void render(sf::RenderWindow& window, const PerspectiveConfig& config, const sf::Transform& canvasTransform, sf::Vector2u canvasSize, int hoveredVPIndex, int activeVPIndex);
};

class PerspectiveSerializer {
public:
    static std::string serialize(const std::vector<PerspectiveConfig>& configs);
    static std::vector<PerspectiveConfig> deserialize(const std::string& data);
};

class PerspectiveManager {
private:
    std::vector<PerspectiveConfig> configs;
    int activeConfigIndex;
    std::vector<std::vector<PerspectiveConfig>> undoStack;
    std::vector<std::vector<PerspectiveConfig>> redoStack;

public:
    PerspectiveManager();
    void init();

    PerspectiveConfig* getActiveConfig();
    const PerspectiveConfig* getActiveConfigReadOnly() const;
    int getActiveConfigIndex() const;
    void setActiveConfig(int index);

    void addConfig(PerspectiveMode mode, sf::Vector2u canvasSize);
    void removeConfig(int index);
    void loadPreset(const std::string& presetName, sf::Vector2u canvasSize);

    void saveUndoState();
    void undo();
    void redo();

    std::vector<std::string> getPresetNames() const;
};