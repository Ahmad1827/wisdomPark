#pragma once
#include <SFML/Graphics.hpp>
#include <random>
#include <vector>
#include <string>
#include <fstream>
#include <map>
#include <queue>

class AIHelper {
public:
    struct PlacedItem {
        int datasetIndex;
        std::string category;
        sf::FloatRect bounds;
    };

private:
    std::mt19937 rng;
    sf::CircleShape mascot;
    bool active;
    bool isGenerating;
    bool isTrained;
    bool terrainEnabled;

    int width;
    int height;
    float currentX;
    float currentY;

    struct Template {
        std::string name;
        std::string category;
        float scale;
        int width;
        int height;
        std::vector<std::string> pixels;
    };

    std::vector<PlacedItem> history;
    std::map<int, std::vector<PlacedItem>> frameMemory;
    int currentMemoryFrame;

    std::vector<Template> datasetTemplates;
    std::vector<int> grid;
    std::vector<int> drawOrder;

    int currentDrawIndex;
    sf::FloatRect currentBounds;
    sf::Color baseColor;
    sf::Color lightColor;
    sf::Color darkColor;

    std::string currentTheme;
    std::map<std::string, std::vector<std::string>> dynamicThesaurus;
    std::vector<std::string> stopWords;
    std::vector<std::string> fillWords;
    std::vector<int> terrainGrid;
    int terrainWidth;
    int terrainHeight;
    float terrainX;
    float terrainY;
    float terrainPixelSize;
    sf::Color terrainColor;
    int currentTerrainDrawIndex;

    struct Node {
        int x, y;
        float g, h;
        Node* parent;
        float f() const { return g + h; }
    };

    void clearGrid();
    std::vector<std::string> generateDynamicBlueprint(std::mt19937& rng);
    std::vector<std::string> generateWFCBuilding(std::mt19937& rng);
    void generateFromTemplate(std::mt19937& rng, const std::vector<std::string>& blueprint);
    void applyShading();
    void applyOutline();
    void generateTerrainPatch(std::mt19937& rng, sf::FloatRect itemBounds);

public:
    AIHelper();
    void toggle();
    bool isActive() const;
    void toggleTerrain();
    bool isTerrainEnabled() const;
    sf::FloatRect getBounds() const;
    void draw(sf::RenderWindow& window);
    void parseCommand(const std::string& input, int& outQuantity, bool& outIsFill, std::string& outTheme);
    void trainOnDataset(const std::string& filename);
    void loadThesaurus(const std::string& filename);
    std::string startGeneratingComplexArt(sf::FloatRect bounds, const sf::Image& currentCanvas, bool isAnimation = false);
    void update(sf::RenderTexture& canvas);

    void setFrame(int frameIndex);
    void clearAllMemory();
    void clearCurrentFrame();
    void forceFinish(sf::RenderTexture& canvas);

    void setTheme(const std::string& theme);
    std::string getTheme() const;

    void cancelSlowDraw();
    float getArtWidth() const;
    float getArtHeight() const;
    void stampOnCanvas(sf::RenderTexture& canvas, float drawX, float drawY);

    void generatePath(sf::RenderTexture& canvas, sf::Vector2f start, sf::Vector2f end, sf::FloatRect bounds);

    const std::vector<PlacedItem>& getHistory() const { return history; }
};