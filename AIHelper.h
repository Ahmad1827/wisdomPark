#pragma once
#include <SFML/Graphics.hpp>
#include <random>
#include <vector>
#include <string>
#include <fstream>

class AIHelper {
private:
    sf::CircleShape mascot;
    bool active;
    bool isGenerating;
    bool isTrained;

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

    struct PlacedItem {
        int datasetIndex;
        std::string category;
        sf::FloatRect bounds;
    };

    std::vector<PlacedItem> history;
    std::vector<Template> datasetTemplates;
    std::vector<int> grid;
    std::vector<int> drawOrder;

    int currentDrawIndex;
    sf::FloatRect currentBounds;
    sf::Color baseColor;
    sf::Color lightColor;
    sf::Color darkColor;

    void clearGrid();
    std::vector<std::string> generateDynamicBlueprint(std::mt19937& rng);
    void generateFromTemplate(std::mt19937& rng, const std::vector<std::string>& blueprint);
    void applyShading();
    void applyOutline();

public:
    AIHelper();
    void toggle();
    bool isActive() const;
    sf::FloatRect getBounds() const;
    void draw(sf::RenderWindow& window);

    void trainOnDataset(const std::string& filename);
    std::string startGeneratingComplexArt(sf::FloatRect bounds);
    void update(sf::RenderTexture& canvas);
};